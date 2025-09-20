#include "RenderableManager.h"
#include <unordered_map>
#include <algorithm>
#include <variant>

#include "JobSystem.h"
#include "RenderComponent.h"
#include "Profiler.h"
#include "SIMDWrapper.h"

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
    };


    class InterpolationAndBatchJob : public IJob
    {
    public:
        const Renderable* prevFrameStart;
        const Renderable* prevFrameEnd;
        const Renderable* currFrameStart;
        const Renderable* currFrameEnd;
        float alpha;


        ThreadLocalBatchResult* result;
        InterpolationAndBatchJob() = default;


        InterpolationAndBatchJob(const Renderable* pStart, const Renderable* pEnd,
                                 const Renderable* cStart, const Renderable* cEnd,
                                 float a, ThreadLocalBatchResult* res)
            : prevFrameStart(pStart), prevFrameEnd(pEnd),
              currFrameStart(cStart), currFrameEnd(cEnd),
              alpha(a), result(res)
        {
        }


        void Execute() override
        {
            auto& simd = SIMD::GetInstance();
            const float one_minus_alpha = 1.0f - alpha;

            auto prevIt = prevFrameStart;
            auto currIt = currFrameStart;

            
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


                std::visit([&, this](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, SpriteRenderData>)
                    {
                        const SpriteRenderData& spriteData = arg;
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
                            interpolatedTransform.position, interpolatedTransform.scale.x,
                            interpolatedTransform.scale.y,
                            sinf(interpolatedTransform.rotation), cosf(interpolatedTransform.rotation));
                    }
                    else if constexpr (std::is_same_v<T, TextRenderData>)
                    {
                        const TextRenderData& textData = arg;
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
                            interpolatedTransform.position, interpolatedTransform.scale.x,
                            interpolatedTransform.scale.y,
                            sinf(interpolatedTransform.rotation), cosf(interpolatedTransform.rotation));
                        group.texts.emplace_back(textData.text);
                    }
                }, currIt->data);

                ++currIt;
            }
        }
    };
}


void RenderableManager::SubmitFrame(std::vector<Renderable>&& frameData)
{
    std::scoped_lock lk(mutex);

    
    auto newFrame = std::make_shared<std::vector<Renderable>>(std::move(frameData));

    
    m_prevFrame = std::move(m_currFrame);
    m_currFrame = std::move(newFrame);

    m_prevStateTime = m_currStateTime;
    m_currStateTime = std::chrono::steady_clock::now();
}

const std::vector<RenderPacket>& RenderableManager::GetInterpolationData()
{
    PROFILE_SCOPE("RenderableManager::GetInterpolationData - Total");

    std::shared_ptr<std::vector<Renderable>> prevFrameSnapshot;
    std::shared_ptr<std::vector<Renderable>> currFrameSnapshot;
    std::chrono::steady_clock::time_point prevStateTime_copy, currStateTime_copy;

    {
        std::scoped_lock lk(mutex);
        prevFrameSnapshot = m_prevFrame;   
        currFrameSnapshot = m_currFrame;   
        prevStateTime_copy = m_prevStateTime;
        currStateTime_copy = m_currStateTime;
    }

    static const std::vector<Renderable> kEmpty;
    const auto& prevFrameRenderables_copy = prevFrameSnapshot ? *prevFrameSnapshot : kEmpty;
    const auto& currFrameRenderables_copy = currFrameSnapshot ? *currFrameSnapshot : kEmpty;


    float alpha = 0.0f;
    auto renderTime = std::chrono::steady_clock::now();


    auto stateDuration = std::chrono::duration<float>(currStateTime_copy - prevStateTime_copy);


    auto renderDuration = std::chrono::duration<float>(renderTime - currStateTime_copy);

    if (stateDuration.count() > 0.0f)
    {
        alpha = renderDuration.count() / stateDuration.count();
    }


    alpha = std::clamp(alpha, 0.0f, 1.0f);

    m_renderPackets.clear();
    m_transformArena->Reverse();
    m_textArena->Reverse();
    m_spriteGroupIndices.clear();
    m_textGroupIndices.clear();
    m_spriteBatchGroups.clear();
    m_textBatchGroups.clear();


    if (currFrameRenderables_copy.empty())
    {
        return m_renderPackets;
    }


    PROFILE_SCOPE("Stage 1: Parallel Interpolation & Batching");

    auto& jobSystem = JobSystem::GetInstance();
    int numJobs = jobSystem.GetThreadCount();

    if (currFrameRenderables_copy.size() < 128)
    {
        numJobs = 1;
    }

    std::vector<JobHandle> jobHandles;
    jobHandles.reserve(numJobs);


    std::vector<InterpolationAndBatchJob> jobs;
    jobs.reserve(numJobs);

    std::vector<ThreadLocalBatchResult> threadResults(numJobs);

    size_t totalSize = currFrameRenderables_copy.size();
    size_t chunkSize = (totalSize + numJobs - 1) / numJobs;

    
    std::vector<std::pair<size_t, size_t>> segments;
    segments.reserve(numJobs);
    size_t pos = 0;
    while (pos < totalSize)
    {
        size_t end = std::min(pos + chunkSize, totalSize);
        if (end < totalSize)
        {
            auto eid = currFrameRenderables_copy[end - 1].entityId;
            while (end < totalSize && currFrameRenderables_copy[end].entityId == eid)
            {
                ++end;
            }
        }
        segments.emplace_back(pos, end);
        pos = end;
    }

    
    int segCount = static_cast<int>(segments.size());
    threadResults.resize(segCount);
    for (int si = 0; si < segCount; ++si)
    {
        size_t start = segments[si].first;
        size_t end = segments[si].second;

        
        size_t chunkItems = end - start;
        auto& tr = threadResults[si];
        tr.spriteGroupIndices.reserve(static_cast<size_t>(std::max<size_t>(8, chunkItems / 4)));
        tr.textGroupIndices.reserve(static_cast<size_t>(std::max<size_t>(4, chunkItems / 8)));
        tr.spriteBatchGroups.reserve(static_cast<size_t>(std::max<size_t>(8, chunkItems / 4)));
        tr.textBatchGroups.reserve(static_cast<size_t>(std::max<size_t>(4, chunkItems / 8)));

        jobs.emplace_back(
            prevFrameRenderables_copy.data(), prevFrameRenderables_copy.data() + prevFrameRenderables_copy.size(),
            currFrameRenderables_copy.data() + start, currFrameRenderables_copy.data() + end,
            alpha,
            &tr
        );

        jobHandles.push_back(jobSystem.Schedule(&jobs.back()));
    }
    JobSystem::CompleteAll(jobHandles);

    
    size_t totalSpriteGroups = 0;
    size_t totalTextGroups = 0;
    for (const auto& r : threadResults)
    {
        totalSpriteGroups += r.spriteBatchGroups.size();
        totalTextGroups += r.textBatchGroups.size();
    }
    m_spriteBatchGroups.reserve(m_spriteBatchGroups.size() + totalSpriteGroups);
    m_textBatchGroups.reserve(m_textBatchGroups.size() + totalTextGroups);


    PROFILE_SCOPE("Stage 2: Serial Merge");


    for (auto& result : threadResults)
    {
        for (auto& threadGroup : result.spriteBatchGroups)
        {
            FastSpriteBatchKey key(threadGroup.image, threadGroup.material, threadGroup.color,
                                   static_cast<ECS::FilterQuality>(threadGroup.filterQuality),
                                   static_cast<ECS::WrapMode>(threadGroup.wrapMode));

            auto it = m_spriteGroupIndices.find(key);
            if (it == m_spriteGroupIndices.end())
            {
                size_t newIndex = m_spriteBatchGroups.size();
                m_spriteGroupIndices[key] = newIndex;
                m_spriteBatchGroups.push_back(std::move(threadGroup));
            }
            else
            {
                auto& masterGroup = m_spriteBatchGroups[it->second];
                masterGroup.transforms.insert(masterGroup.transforms.end(), threadGroup.transforms.begin(),
                                              threadGroup.transforms.end());
            }
        }
    }


    for (auto& result : threadResults)
    {
        for (auto& threadGroup : result.textBatchGroups)
        {
            FastTextBatchKey key(threadGroup.typeface, threadGroup.fontSize, threadGroup.alignment, threadGroup.color);

            auto it = m_textGroupIndices.find(key);
            if (it == m_textGroupIndices.end())
            {
                size_t newIndex = m_textBatchGroups.size();
                m_textGroupIndices[key] = newIndex;
                m_textBatchGroups.push_back(std::move(threadGroup));
            }
            else
            {
                auto& masterGroup = m_textBatchGroups[it->second];
                masterGroup.transforms.insert(masterGroup.transforms.end(), threadGroup.transforms.begin(),
                                              threadGroup.transforms.end());
                masterGroup.texts.insert(masterGroup.texts.end(), threadGroup.texts.begin(), threadGroup.texts.end());
            }
        }
    }


    PROFILE_SCOPE("Stage 3: Packing & Sorting");

    m_renderPackets.reserve(m_spriteBatchGroups.size() + m_textBatchGroups.size());


    for (const auto& group : m_spriteBatchGroups)
    {
        const size_t count = group.transforms.size();
        if (count == 0) continue;

        RenderableTransform* transformBuffer = m_transformArena->Allocate(count);
        std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

        m_renderPackets.emplace_back(RenderPacket{
            .zIndex = group.zIndex,
            .batchData = SpriteBatch{
                .material = group.material, .image = sk_ref_sp(group.image), .sourceRect = group.sourceRect,
                .color = group.color, .transforms = transformBuffer, .filterQuality = group.filterQuality,
                .wrapMode = group.wrapMode, .ppuScaleFactor = group.ppuScaleFactor, .count = count
            }
        });
    }


    for (const auto& group : m_textBatchGroups)
    {
        const size_t count = group.transforms.size();
        if (count == 0) continue;

        RenderableTransform* transformBuffer = m_transformArena->Allocate(count);
        std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

        std::string* textBuffer = m_textArena->Allocate(count);
        for (size_t i = 0; i < count; ++i)
        {
            new(&textBuffer[i]) std::string(group.texts[i]);
        }

        m_renderPackets.emplace_back(RenderPacket{
            .zIndex = group.zIndex,
            .batchData = TextBatch{
                .typeface = sk_ref_sp(group.typeface), .fontSize = group.fontSize, .color = group.color,
                .texts = textBuffer, .alignment = static_cast<int>(group.alignment),
                .transforms = transformBuffer, .count = count
            }
        });
    }


    std::ranges::sort(m_renderPackets, [](const RenderPacket& a, const RenderPacket& b)
    {
        return a.zIndex < b.zIndex;
    });

    return m_renderPackets;
}

RenderableManager::RenderableManager()
{
    auto now = std::chrono::steady_clock::now();
    m_prevStateTime = now;
    m_currStateTime = now;
}
