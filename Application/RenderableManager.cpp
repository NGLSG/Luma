#include "RenderableManager.h"
#include <unordered_map>
#include <algorithm>
#include <variant>

#include "JobSystem.h"
#include "RenderComponent.h"
#include "Profiler.h"
#include "SIMDWrapper.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"

inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

namespace
{
    struct ThreadLocalBatchResult
    {
        std::unordered_map<FastSpriteBatchKey, size_t> spriteGroupIndices;
        std::unordered_map<FastTextBatchKey, size_t> textGroupIndices;

        std::vector<SceneRenderer::BatchGroup> spriteBatchGroups;
        std::vector<SceneRenderer::BatchGroup> textBatchGroups;
        std::vector<RenderPacket> rawDrawPackets;
    };

    class InterpolationAndBatchJob : public IJob
    {
    public:
        const Renderable* prevFrameStart;
        const Renderable* prevFrameEnd;
        const Renderable* currFrameStart;
        const Renderable* currFrameEnd;
        float alpha;
        bool shouldInterpolate;

        ThreadLocalBatchResult* result;
        InterpolationAndBatchJob() = default;

        InterpolationAndBatchJob(const Renderable* pStart, const Renderable* pEnd,
                                 const Renderable* cStart, const Renderable* cEnd,
                                 float a, bool interpolate, ThreadLocalBatchResult* res)
            : prevFrameStart(pStart), prevFrameEnd(pEnd),
              currFrameStart(cStart), currFrameEnd(cEnd),
              alpha(a), shouldInterpolate(interpolate), result(res)
        {
        }

        void Execute() override
        {
            auto& simd = SIMD::GetInstance();
            const float one_minus_alpha = 1.0f - alpha;

            auto prevIt = prevFrameStart;
            auto currIt = currFrameStart;

            if (!shouldInterpolate)
            {
                processCurrentFrameOnly();
                return;
            }

            if (currIt != currFrameEnd)
            {
                auto firstId = currIt->entityId;
                while (prevIt != prevFrameEnd && prevIt->entityId < firstId)
                {
                    ++prevIt;
                }
            }

            while (currIt != currFrameEnd)
            {
                while (prevIt != prevFrameEnd && prevIt->entityId < currIt->entityId)
                {
                    ++prevIt;
                }

                ECS::TransformComponent interpolatedTransform = currIt->transform;

                if (prevIt != prevFrameEnd && prevIt->entityId == currIt->entityId)
                {
                    const float prevPos[2] = {prevIt->transform.position.x, prevIt->transform.position.y};
                    const float currPos[2] = {currIt->transform.position.x, currIt->transform.position.y};
                    float resultPos[2];

                    const float prevScale[2] = {prevIt->transform.scale.x, prevIt->transform.scale.y};
                    const float currScale[2] = {currIt->transform.scale.x, currIt->transform.scale.y};
                    float resultScale[2];

                    float term1[2], term2[2];

                    simd.VectorScalarMultiply(prevPos, one_minus_alpha, term1, 2);
                    simd.VectorScalarMultiply(currPos, alpha, term2, 2);
                    simd.VectorAdd(term1, term2, resultPos, 2);

                    simd.VectorScalarMultiply(prevScale, one_minus_alpha, term1, 2);
                    simd.VectorScalarMultiply(currScale, alpha, term2, 2);
                    simd.VectorAdd(term1, term2, resultScale, 2);

                    interpolatedTransform.position = {resultPos[0], resultPos[1]};
                    interpolatedTransform.scale = {resultScale[0], resultScale[1]};
                    interpolatedTransform.rotation =
                        Lerp(prevIt->transform.rotation, currIt->transform.rotation, alpha);

                    ++prevIt;
                }

                processRenderable(currIt, interpolatedTransform);
                ++currIt;
            }
        }

    private:
        void processCurrentFrameOnly()
        {
            auto currIt = currFrameStart;
            while (currIt != currFrameEnd)
            {
                processRenderable(currIt, currIt->transform);
                ++currIt;
            }
        }

        void processRenderable(const Renderable* currIt, const ECS::TransformComponent& transform)
        {
            std::visit([&, this](auto&& arg)
            {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, SpriteRenderData>)
                {
                    processSpriteData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, TextRenderData>)
                {
                    processTextData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawButtonRenderData>)
                {
                    processButtonData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawInputTextRenderData>)
                {
                    processInputTextData(currIt, transform, arg);
                }
            }, currIt->data);
        }

        void processButtonData(const Renderable* currIt, const ECS::TransformComponent& transform,
                               const RawButtonRenderData& buttonData)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    data = buttonData
                ]
            (SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = data.rect;
                    worldRect.x = trans.position.x - data.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - data.rect.Height() * 0.5f;

                    ECS::Color tintColor;
                    switch (data.currentState)
                    {
                    case ECS::ButtonState::Hovered: tintColor = data.hoverColor;
                        break;
                    case ECS::ButtonState::Pressed: tintColor = data.pressedColor;
                        break;
                    case ECS::ButtonState::Disabled: tintColor = data.disabledColor;
                        break;
                    case ECS::ButtonState::Normal:
                    default: tintColor = data.normalColor;
                        break;
                    }

                    SkPaint paint;
                    paint.setAntiAlias(true);
                    SkRect skRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), worldRect.Height());

                    if (data.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(tintColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeRectXY(skRect, data.roundness, data.roundness), true);
                        canvas->drawImageRect(
                            data.backgroundImage,
                            skRect,
                            SkSamplingOptions(SkFilterMode::kLinear)
                        );
                        canvas->restore();
                    }
                    else
                    {
                        paint.setColor4f({tintColor.r, tintColor.g, tintColor.b, tintColor.a}, nullptr);
                        canvas->drawRRect(SkRRect::MakeRectXY(skRect, data.roundness, data.roundness), paint);
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processInputTextData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                  const RawInputTextRenderData& inputTextData)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    data = inputTextData
                ]
            (SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = data.rect;
                    worldRect.x = trans.position.x - data.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - data.rect.Height() * 0.5f;

                    SkPaint paint;
                    paint.setAntiAlias(true);
                    SkRect skRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), worldRect.Height());

                    ECS::Color bgColor = data.isReadOnly
                                             ? data.readOnlyBackgroundColor
                                             : (data.isFocused
                                                    ? data.focusedBackgroundColor
                                                    : data.normalBackgroundColor);

                    if (data.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(bgColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeRectXY(skRect, data.roundness, data.roundness), true);
                        canvas->drawImageRect(data.backgroundImage, skRect, SkSamplingOptions(SkFilterMode::kLinear));
                        canvas->restore();
                    }
                    else
                    {
                        paint.setColor4f({bgColor.r, bgColor.g, bgColor.b, bgColor.a});
                        canvas->drawRRect(SkRRect::MakeRectXY(skRect, data.roundness, data.roundness), paint);
                    }

                    bool isShowingPlaceholder = data.inputBuffer.empty() && !data.isFocused;
                    const auto& textToDrawData = isShowingPlaceholder ? data.placeholder : data.text;

                    if (!textToDrawData.typeface) return;

                    SkFont font(textToDrawData.typeface, textToDrawData.fontSize);
                    SkFontMetrics metrics{};
                    font.getMetrics(&metrics);
                    float textY = worldRect.y + worldRect.Height() / 2.0f - (metrics.fAscent + metrics.fDescent) / 2.0f;

                    std::string displayText = isShowingPlaceholder
                                                  ? textToDrawData.text
                                                  : (data.isPasswordField
                                                         ? std::string(data.inputBuffer.length(), '*')
                                                         : data.inputBuffer);

                    SkPaint textPaint;
                    textPaint.setColor4f({
                        textToDrawData.color.r, textToDrawData.color.g, textToDrawData.color.b, textToDrawData.color.a
                    });

                    canvas->save();
                    canvas->clipRect(skRect);
                    canvas->drawString(displayText.c_str(), worldRect.x + 5.0f, textY, font, textPaint);

                    if (data.isFocused && data.isCursorVisible)
                    {
                        const std::string& textForMeasurement = data.isPasswordField ? displayText : data.inputBuffer;
                        const size_t safeCursorPos = std::min<size_t>(data.cursorPosition, textForMeasurement.length());
                        std::string textBeforeCursor = textForMeasurement.substr(0, safeCursorPos);

                        SkRect bounds{};
                        font.measureText(textBeforeCursor.c_str(), textBeforeCursor.size(), SkTextEncoding::kUTF8,
                                         &bounds);

                        float cursorX = worldRect.x + 5.0f + bounds.width();

                        SkPaint cursorPaint;
                        cursorPaint.setColor4f({
                            data.cursorColor.r, data.cursorColor.g, data.cursorColor.b, data.cursorColor.a
                        });
                        cursorPaint.setStrokeWidth(1.0f);

                        canvas->drawLine(cursorX, textY + metrics.fAscent, cursorX, textY + metrics.fDescent,
                                         cursorPaint);
                    }
                    canvas->restore();
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processSpriteData(const Renderable* currIt, const ECS::TransformComponent& transform,
                               const SpriteRenderData& spriteData)
        {
            FastSpriteBatchKey key(spriteData.image, spriteData.material, spriteData.color,
                                   static_cast<ECS::FilterQuality>(spriteData.filterQuality),
                                   static_cast<ECS::WrapMode>(spriteData.wrapMode));

            auto keyIt = result->spriteGroupIndices.find(key);
            size_t groupIndex;

            if (keyIt == result->spriteGroupIndices.end())
            {
                groupIndex = result->spriteBatchGroups.size();
                result->spriteGroupIndices[key] = groupIndex;

                result->spriteBatchGroups.emplace_back();
                auto& group = result->spriteBatchGroups.back();
                group.image = const_cast<SkImage*>(spriteData.image);
                group.material = spriteData.material;
                group.sourceRect = spriteData.sourceRect;
                group.color = spriteData.color;
                group.zIndex = currIt->zIndex;
                group.filterQuality = spriteData.filterQuality;
                group.wrapMode = spriteData.wrapMode;
                group.ppuScaleFactor = spriteData.ppuScaleFactor;
                group.transforms.reserve(32);
            }
            else
            {
                groupIndex = keyIt->second;
            }

            result->spriteBatchGroups[groupIndex].transforms.emplace_back(
                transform.position, transform.scale.x, transform.scale.y,
                sinf(transform.rotation), cosf(transform.rotation));
        }

        void processTextData(const Renderable* currIt, const ECS::TransformComponent& transform,
                             const TextRenderData& textData)
        {
            FastTextBatchKey key(const_cast<SkTypeface*>(textData.typeface), textData.fontSize,
                                 static_cast<TextAlignment>(textData.alignment), textData.color);

            auto keyIt = result->textGroupIndices.find(key);
            size_t groupIndex;

            if (keyIt == result->textGroupIndices.end())
            {
                groupIndex = result->textBatchGroups.size();
                result->textGroupIndices[key] = groupIndex;

                result->textBatchGroups.emplace_back();
                auto& group = result->textBatchGroups.back();
                group.typeface = const_cast<SkTypeface*>(textData.typeface);
                group.fontSize = textData.fontSize;
                group.color = textData.color;
                group.alignment = static_cast<TextAlignment>(textData.alignment);
                group.zIndex = currIt->zIndex;
                group.transforms.reserve(16);
                group.texts.reserve(16);
            }
            else
            {
                groupIndex = keyIt->second;
            }

            auto& group = result->textBatchGroups[groupIndex];
            group.transforms.emplace_back(
                transform.position, transform.scale.x, transform.scale.y,
                sinf(transform.rotation), cosf(transform.rotation));
            group.texts.emplace_back(textData.text);
        }
    };
}

void RenderableManager::SubmitFrame(DynamicArray<Renderable>&& frameData)
{
    while (isUpdatingFrames.exchange(true, std::memory_order_acquire))
    {
    }

    {
        auto currView = currFrame.GetView();
        prevFrame.ClearAndModify([&](auto& proxy)
        {
            proxy.Reserve(currView.Size());
            for (size_t i = 0; i < currView.Size(); ++i)
            {
                proxy.PushBack(currView[i]);
            }
        });
    }

    {
        auto newView = frameData.GetView();
        currFrame.ClearAndModify([&](auto& proxy)
        {
            proxy.Reserve(newView.Size());
            for (size_t i = 0; i < newView.Size(); ++i)
            {
                proxy.PushBack(newView[i]);
            }
        });
    }

    prevStateTime.store(currStateTime.load(std::memory_order_relaxed), std::memory_order_relaxed);
    currStateTime.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);

    prevFrameVersion.store(currFrameVersion.load(std::memory_order_relaxed), std::memory_order_relaxed);
    currFrameVersion.fetch_add(1, std::memory_order_relaxed);

    isUpdatingFrames.store(false, std::memory_order_release);
}

void RenderableManager::SubmitFrame(std::vector<Renderable>&& frameData)
{
    DynamicArray<Renderable> dynamicFrameData;
    dynamicFrameData.ClearAndModify([&frameData](auto& proxy)
    {
        proxy.Reserve(frameData.size());
        for (auto&& renderable : frameData)
        {
            proxy.EmplaceBack(std::move(renderable));
        }
    });

    SubmitFrame(std::move(dynamicFrameData));
}

const std::vector<RenderPacket>& RenderableManager::GetInterpolationData()
{
    while (isUpdatingFrames.load(std::memory_order_acquire))
    {
    }

    if (!needsRebuild())
    {
        return packetBuffers[activeBufferIndex.load(std::memory_order_relaxed)];
    }

    auto prevFrameView = prevFrame.GetView();
    auto currFrameView = currFrame.GetView();

    bool hasPrevFrame = !prevFrameView.IsEmpty();
    bool hasCurrFrame = !currFrameView.IsEmpty();

    if (!hasPrevFrame && !hasCurrFrame)
    {
        const int buildIndex = (activeBufferIndex.load(std::memory_order_relaxed) ^ 1);
        auto& outPackets = packetBuffers[buildIndex];
        outPackets.clear();
        activeBufferIndex.store(buildIndex, std::memory_order_release);
        updateCacheState();
        return outPackets;
    }

    bool shouldInterpolate = hasPrevFrame && hasCurrFrame;
    auto baseFrameView = hasCurrFrame ? currFrameView : prevFrameView;

    float alpha = 0.0f;
    if (shouldInterpolate)
    {
        float ext = m_externalAlpha.load(std::memory_order_relaxed);
        if (ext >= 0.0f && ext <= 1.0f)
        {
            alpha = ext;
        }
        else
        {
            auto renderTime = std::chrono::steady_clock::now();
            auto prevTime = prevStateTime.load(std::memory_order_relaxed);
            auto currTime = currStateTime.load(std::memory_order_relaxed);

            auto stateDuration = std::chrono::duration<float>(currTime - prevTime);
            auto renderDuration = std::chrono::duration<float>(renderTime - currTime);

            if (stateDuration.count() > 0.0f)
            {
                alpha = renderDuration.count() / stateDuration.count();
            }
            alpha = std::clamp(alpha, 0.0f, 1.0f);
        }
    }

    const int buildIndex = (activeBufferIndex.load(std::memory_order_relaxed) ^ 1);
    auto& outPackets = packetBuffers[buildIndex];
    outPackets.clear();
    transformArenas[buildIndex]->Reverse();
    textArenas[buildIndex]->Reverse();
    spriteGroupIndices.clear();
    textGroupIndices.clear();
    spriteBatchGroups.clear();
    textBatchGroups.clear();

    if (baseFrameView.IsEmpty())
    {
        activeBufferIndex.store(buildIndex, std::memory_order_release);
        updateCacheState();
        return outPackets;
    }

    auto& jobSystem = JobSystem::GetInstance();
    int numJobs = jobSystem.GetThreadCount();

    if (baseFrameView.Size() < 128)
    {
        numJobs = 1;
    }

    std::vector<JobHandle> jobHandles;
    jobHandles.reserve(numJobs);

    std::vector<InterpolationAndBatchJob> jobs;
    std::vector<ThreadLocalBatchResult> threadResults(numJobs);

    size_t totalSize = baseFrameView.Size();
    size_t chunkSize = (totalSize + numJobs - 1) / numJobs;

    std::vector<std::pair<size_t, size_t>> segments;
    segments.reserve(numJobs);

    size_t pos = 0;
    while (pos < totalSize)
    {
        size_t end = std::min(pos + chunkSize, totalSize);
        if (end < totalSize)
        {
            auto eid = baseFrameView[end - 1].entityId;
            while (end < totalSize && baseFrameView[end].entityId == eid)
            {
                ++end;
            }
        }
        segments.emplace_back(pos, end);
        pos = end;
    }

    jobs.reserve(segments.size());
    for (size_t si = 0; si < segments.size(); ++si)
    {
        size_t start = segments[si].first;
        size_t end = segments[si].second;

        const Renderable* prevStart = nullptr;
        const Renderable* prevEnd = nullptr;
        const Renderable* currStart = nullptr;
        const Renderable* currEnd = nullptr;

        if (shouldInterpolate)
        {
            prevStart = prevFrameView.Data();
            prevEnd = prevFrameView.Data() + prevFrameView.Size();
            currStart = currFrameView.Data() + start;
            currEnd = currFrameView.Data() + end;
        }
        else
        {
            currStart = baseFrameView.Data() + start;
            currEnd = baseFrameView.Data() + end;
        }

        size_t chunkItems = end - start;
        auto& tr = threadResults[si];
        tr.spriteGroupIndices.reserve(std::max<size_t>(8, chunkItems / 4));
        tr.textGroupIndices.reserve(std::max<size_t>(4, chunkItems / 8));
        tr.spriteBatchGroups.reserve(std::max<size_t>(8, chunkItems / 4));
        tr.textBatchGroups.reserve(std::max<size_t>(4, chunkItems / 8));

        jobs.emplace_back(
            prevStart, prevEnd, currStart, currEnd,
            alpha, shouldInterpolate,
            &tr
        );

        jobHandles.push_back(jobSystem.Schedule(&jobs.back()));
    }
    JobSystem::CompleteAll(jobHandles);

    size_t totalSpriteGroups = 0;
    size_t totalTextGroups = 0;
    for (const auto& result : threadResults)
    {
        totalSpriteGroups += result.spriteBatchGroups.size();
        totalTextGroups += result.textBatchGroups.size();
    }

    spriteBatchGroups.reserve(totalSpriteGroups);
    textBatchGroups.reserve(totalTextGroups);

    for (const auto& result : threadResults)
    {
        for (const auto& threadGroup : result.spriteBatchGroups)
        {
            FastSpriteBatchKey key(threadGroup.image, threadGroup.material, threadGroup.color,
                                   static_cast<ECS::FilterQuality>(threadGroup.filterQuality),
                                   static_cast<ECS::WrapMode>(threadGroup.wrapMode));

            auto it = spriteGroupIndices.find(key);
            if (it == spriteGroupIndices.end())
            {
                size_t newIndex = spriteBatchGroups.size();
                spriteGroupIndices[key] = newIndex;
                spriteBatchGroups.push_back(threadGroup);
            }
            else
            {
                auto& masterGroup = spriteBatchGroups[it->second];
                masterGroup.transforms.insert(masterGroup.transforms.end(),
                                              threadGroup.transforms.begin(),
                                              threadGroup.transforms.end());
            }
        }
    }

    for (const auto& result : threadResults)
    {
        for (const auto& threadGroup : result.textBatchGroups)
        {
            FastTextBatchKey key(threadGroup.typeface, threadGroup.fontSize,
                                 threadGroup.alignment, threadGroup.color);

            auto it = textGroupIndices.find(key);
            if (it == textGroupIndices.end())
            {
                size_t newIndex = textBatchGroups.size();
                textGroupIndices[key] = newIndex;
                textBatchGroups.push_back(threadGroup);
            }
            else
            {
                auto& masterGroup = textBatchGroups[it->second];
                masterGroup.transforms.insert(masterGroup.transforms.end(),
                                              threadGroup.transforms.begin(),
                                              threadGroup.transforms.end());
                masterGroup.texts.insert(masterGroup.texts.end(),
                                         threadGroup.texts.begin(),
                                         threadGroup.texts.end());
            }
        }
    }

    outPackets.reserve(spriteBatchGroups.size() + textBatchGroups.size());

    for (const auto& group : spriteBatchGroups)
    {
        const size_t count = group.transforms.size();
        if (count == 0) continue;

        RenderableTransform* transformBuffer = transformArenas[buildIndex]->Allocate(count);
        std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

        outPackets.emplace_back(RenderPacket{
            .zIndex = group.zIndex,
            .batchData = SpriteBatch{
                .material = group.material,
                .image = sk_ref_sp(group.image),
                .sourceRect = group.sourceRect,
                .color = group.color,
                .transforms = transformBuffer,
                .filterQuality = group.filterQuality,
                .wrapMode = group.wrapMode,
                .ppuScaleFactor = group.ppuScaleFactor,
                .count = count
            }
        });
    }

    for (const auto& group : textBatchGroups)
    {
        const size_t count = group.transforms.size();
        if (count == 0) continue;

        RenderableTransform* transformBuffer = transformArenas[buildIndex]->Allocate(count);
        std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

        std::string* textBuffer = textArenas[buildIndex]->Allocate(count);
        for (size_t j = 0; j < count; ++j)
        {
            textBuffer[j] = group.texts[j];
        }

        outPackets.emplace_back(RenderPacket{
            .zIndex = group.zIndex,
            .batchData = TextBatch{
                .typeface = sk_ref_sp(group.typeface),
                .fontSize = group.fontSize,
                .color = group.color,
                .texts = textBuffer,
                .alignment = static_cast<int>(group.alignment),
                .transforms = transformBuffer,
                .count = count
            }
        });
    }

    for (const auto& result : threadResults)
    {
        if (!result.rawDrawPackets.empty())
        {
            outPackets.insert(outPackets.end(), result.rawDrawPackets.begin(), result.rawDrawPackets.end());
        }
    }

    std::ranges::sort(outPackets, [](const RenderPacket& a, const RenderPacket& b)
    {
        return a.zIndex < b.zIndex;
    });

    activeBufferIndex.store(buildIndex, std::memory_order_release);
    updateCacheState();
    return outPackets;
}

RenderableManager::RenderableManager()
{
    auto now = std::chrono::steady_clock::now();
    prevStateTime.store(now, std::memory_order_relaxed);
    currStateTime.store(now, std::memory_order_relaxed);
    lastBuiltPrevTime.store(now, std::memory_order_relaxed);
    lastBuiltCurrTime.store(now, std::memory_order_relaxed);
}

bool RenderableManager::needsRebuild() const
{
    auto currentPrevTime = prevStateTime.load(std::memory_order_relaxed);
    auto currentCurrTime = currStateTime.load(std::memory_order_relaxed);
    auto currentPrevVersion = prevFrameVersion.load(std::memory_order_relaxed);
    auto currentCurrVersion = currFrameVersion.load(std::memory_order_relaxed);

    return lastBuiltPrevTime.load(std::memory_order_relaxed) != currentPrevTime ||
        lastBuiltCurrTime.load(std::memory_order_relaxed) != currentCurrTime ||
        lastBuiltPrevFrameVersion.load(std::memory_order_relaxed) != currentPrevVersion ||
        lastBuiltCurrFrameVersion.load(std::memory_order_relaxed) != currentCurrVersion;
}

void RenderableManager::updateCacheState()
{
    lastBuiltPrevTime.store(prevStateTime.load(std::memory_order_relaxed), std::memory_order_relaxed);
    lastBuiltCurrTime.store(currStateTime.load(std::memory_order_relaxed), std::memory_order_relaxed);
    lastBuiltPrevFrameVersion.store(prevFrameVersion.load(std::memory_order_relaxed), std::memory_order_relaxed);
    lastBuiltCurrFrameVersion.store(currFrameVersion.load(std::memory_order_relaxed), std::memory_order_relaxed);
}
