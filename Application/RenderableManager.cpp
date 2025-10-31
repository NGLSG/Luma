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
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"

#include <cstdint>
#include <cstring>

inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

namespace
{
    static inline uint32_t float_bits(float v)
    {
        uint32_t b;
        static_assert(sizeof(float) == sizeof(uint32_t));
        std::memcpy(&b, &v, sizeof(uint32_t));
        return b;
    }

    static inline uint64_t hash_combine_u64(uint64_t seed, uint64_t v)
    {

        const uint64_t kMul = 0x9ddfea08eb382d69ULL;
        uint64_t a = (v ^ seed) * kMul;
        a ^= (a >> 47);
        uint64_t b = (seed ^ a) * kMul;
        b ^= (b >> 47);
        b *= kMul;
        return b;
    }

    static int packet_type_index(const RenderPacket& p)
    {
        return static_cast<int>(p.batchData.index());
    }

    static uint64_t stable_packet_key(const RenderPacket& p)
    {
        uint64_t seed = 0xcbf29ce484222325ULL;
        std::visit([&](auto const& batch)
        {
            using T = std::decay_t<decltype(batch)>;
            if constexpr (std::is_same_v<T, SpriteBatch>)
            {
                seed = hash_combine_u64(seed, reinterpret_cast<uint64_t>(batch.image.get()));
                seed = hash_combine_u64(seed, reinterpret_cast<uint64_t>(batch.material));
                seed = hash_combine_u64(seed, float_bits(batch.sourceRect.fLeft));
                seed = hash_combine_u64(seed, float_bits(batch.sourceRect.fTop));
                seed = hash_combine_u64(seed, float_bits(batch.sourceRect.fRight));
                seed = hash_combine_u64(seed, float_bits(batch.sourceRect.fBottom));
                seed = hash_combine_u64(seed, float_bits(batch.ppuScaleFactor));

                seed = hash_combine_u64(seed, float_bits(batch.color.fR));
                seed = hash_combine_u64(seed, float_bits(batch.color.fG));
                seed = hash_combine_u64(seed, float_bits(batch.color.fB));
                seed = hash_combine_u64(seed, float_bits(batch.color.fA));
                seed = hash_combine_u64(seed, static_cast<uint64_t>(batch.filterQuality));
                seed = hash_combine_u64(seed, static_cast<uint64_t>(batch.wrapMode));
            }
            else if constexpr (std::is_same_v<T, TextBatch>)
            {
                seed = hash_combine_u64(seed, reinterpret_cast<uint64_t>(batch.typeface.get()));
                seed = hash_combine_u64(seed, float_bits(batch.fontSize));
                seed = hash_combine_u64(seed, float_bits(batch.color.fR));
                seed = hash_combine_u64(seed, float_bits(batch.color.fG));
                seed = hash_combine_u64(seed, float_bits(batch.color.fB));
                seed = hash_combine_u64(seed, float_bits(batch.color.fA));
                seed = hash_combine_u64(seed, static_cast<uint64_t>(batch.alignment));
            }
            else if constexpr (std::is_same_v<T, InstanceBatch>)
            {
                seed = hash_combine_u64(seed, reinterpret_cast<uint64_t>(batch.atlasImage.get()));
                seed = hash_combine_u64(seed, float_bits(batch.color.fR));
                seed = hash_combine_u64(seed, float_bits(batch.color.fG));
                seed = hash_combine_u64(seed, float_bits(batch.color.fB));
                seed = hash_combine_u64(seed, float_bits(batch.color.fA));
                seed = hash_combine_u64(seed, static_cast<uint64_t>(batch.filterQuality));
                seed = hash_combine_u64(seed, static_cast<uint64_t>(batch.wrapMode));
            }
            else if constexpr (std::is_same_v<T, RectBatch>)
            {
                seed = hash_combine_u64(seed, float_bits(batch.size.fWidth));
                seed = hash_combine_u64(seed, float_bits(batch.size.fHeight));
                seed = hash_combine_u64(seed, float_bits(batch.color.fR));
                seed = hash_combine_u64(seed, float_bits(batch.color.fG));
                seed = hash_combine_u64(seed, float_bits(batch.color.fB));
                seed = hash_combine_u64(seed, float_bits(batch.color.fA));
            }
            else if constexpr (std::is_same_v<T, CircleBatch>)
            {
                seed = hash_combine_u64(seed, float_bits(batch.radius));
                seed = hash_combine_u64(seed, float_bits(batch.color.fR));
                seed = hash_combine_u64(seed, float_bits(batch.color.fG));
                seed = hash_combine_u64(seed, float_bits(batch.color.fB));
                seed = hash_combine_u64(seed, float_bits(batch.color.fA));
            }
            else if constexpr (std::is_same_v<T, LineBatch>)
            {
                seed = hash_combine_u64(seed, float_bits(batch.width));
                seed = hash_combine_u64(seed, float_bits(batch.color.fR));
                seed = hash_combine_u64(seed, float_bits(batch.color.fG));
                seed = hash_combine_u64(seed, float_bits(batch.color.fB));
                seed = hash_combine_u64(seed, float_bits(batch.color.fA));
            }
            else if constexpr (std::is_same_v<T, ShaderBatch>)
            {
                seed = hash_combine_u64(seed, reinterpret_cast<uint64_t>(batch.material));
                seed = hash_combine_u64(seed, float_bits(batch.size.fWidth));
                seed = hash_combine_u64(seed, float_bits(batch.size.fHeight));
            }
            else if constexpr (std::is_same_v<T, RawDrawBatch>)
            {

                seed = hash_combine_u64(seed, 0xDEADBEEFULL);
            }
        }, p.batchData);
        return seed;
    }
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
                else if constexpr (std::is_same_v<T, RawToggleButtonRenderData>)
                {
                    processToggleButtonData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawRadioButtonRenderData>)
                {
                    processRadioButtonData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawCheckBoxRenderData>)
                {
                    processCheckBoxData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawSliderRenderData>)
                {
                    processSliderData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawComboBoxRenderData>)
                {
                    processComboBoxData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawExpanderRenderData>)
                {
                    processExpanderData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawProgressBarRenderData>)
                {
                    processProgressBarData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawTabControlRenderData>)
                {
                    processTabControlData(currIt, transform, arg);
                }
                else if constexpr (std::is_same_v<T, RawListBoxRenderData>)
                {
                    processListBoxData(currIt, transform, arg);
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

        void processToggleButtonData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                     const RawToggleButtonRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    button = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = button.rect;
                    worldRect.x = trans.position.x - button.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - button.rect.Height() * 0.5f;

                    ECS::Color tintColor = button.normalColor;
                    if (!button.isToggled)
                    {
                        switch (button.currentState)
                        {
                        case ECS::ButtonState::Hovered: tintColor = button.hoverColor;
                            break;
                        case ECS::ButtonState::Pressed: tintColor = button.pressedColor;
                            break;
                        case ECS::ButtonState::Disabled: tintColor = button.disabledColor;
                            break;
                        case ECS::ButtonState::Normal:
                        default: tintColor = button.normalColor;
                            break;
                        }
                    }
                    else
                    {
                        switch (button.currentState)
                        {
                        case ECS::ButtonState::Hovered: tintColor = button.toggledHoverColor;
                            break;
                        case ECS::ButtonState::Pressed: tintColor = button.toggledPressedColor;
                            break;
                        case ECS::ButtonState::Disabled: tintColor = button.disabledColor;
                            break;
                        case ECS::ButtonState::Normal:
                        default: tintColor = button.toggledColor;
                            break;
                        }
                    }

                    SkRect skRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), worldRect.Height());
                    SkPaint paint;
                    paint.setAntiAlias(true);

                    if (button.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(tintColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeRectXY(skRect, button.roundness, button.roundness), true);
                        canvas->drawImageRect(button.backgroundImage, skRect, SkSamplingOptions(SkFilterMode::kLinear));
                        canvas->restore();
                    }
                    else
                    {
                        paint.setColor4f({tintColor.r, tintColor.g, tintColor.b, tintColor.a}, nullptr);
                        canvas->drawRRect(SkRRect::MakeRectXY(skRect, button.roundness, button.roundness), paint);
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processRadioButtonData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                    const RawRadioButtonRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    radio = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = radio.rect;
                    worldRect.x = trans.position.x - radio.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - radio.rect.Height() * 0.5f;

                    const float circleDiameter = std::min(worldRect.Height(), worldRect.Width() * 0.4f);
                    const float circleRadius = circleDiameter * 0.5f;
                    const float padding = 6.0f;
                    const float circleCenterX = worldRect.x + circleRadius + padding;
                    const float circleCenterY = worldRect.y + worldRect.Height() * 0.5f;

                    ECS::Color baseColor = radio.normalColor;
                    if (radio.currentState == ECS::ButtonState::Hovered) baseColor = radio.hoverColor;
                    if (radio.isSelected) baseColor = radio.selectedColor;
                    if (radio.currentState == ECS::ButtonState::Disabled) baseColor = radio.disabledColor;

                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setColor4f({baseColor.r, baseColor.g, baseColor.b, baseColor.a});

                    SkRect circleRect = SkRect::MakeXYWH(circleCenterX - circleRadius,
                                                         circleCenterY - circleRadius,
                                                         circleDiameter,
                                                         circleDiameter);

                    if (radio.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(baseColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeOval(circleRect), true);
                        canvas->drawImageRect(radio.backgroundImage, circleRect,
                                              SkSamplingOptions(SkFilterMode::kLinear));
                        canvas->restore();
                    }
                    else
                    {
                        canvas->drawCircle(circleCenterX, circleCenterY, circleRadius, paint);
                    }

                    if (radio.isSelected)
                    {
                        const float indicatorRadius = circleRadius * 0.55f;
                        SkRect indicatorRect = SkRect::MakeXYWH(circleCenterX - indicatorRadius,
                                                                circleCenterY - indicatorRadius,
                                                                indicatorRadius * 2.0f,
                                                                indicatorRadius * 2.0f);

                        if (radio.selectionImage)
                        {
                            canvas->drawImageRect(radio.selectionImage, indicatorRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear));
                        }
                        else
                        {
                            SkPaint indicatorPaint;
                            indicatorPaint.setAntiAlias(true);
                            indicatorPaint.setColor4f({
                                radio.indicatorColor.r, radio.indicatorColor.g,
                                radio.indicatorColor.b, radio.indicatorColor.a
                            });
                            canvas->drawCircle(circleCenterX, circleCenterY, indicatorRadius, indicatorPaint);
                        }
                    }

                    if (radio.label.typeface)
                    {
                        SkFont font(radio.label.typeface, radio.label.fontSize);
                        font.setEdging(SkFont::Edging::kAntiAlias);

                        SkPaint textPaint;
                        textPaint.setAntiAlias(true);
                        textPaint.setColor4f({
                            radio.label.color.r, radio.label.color.g, radio.label.color.b, radio.label.color.a
                        });

                        SkFontMetrics metrics;
                        font.getMetrics(&metrics);
                        const float baseline = circleCenterY - (metrics.fAscent + metrics.fDescent) * 0.5f;
                        const float textStartX = circleCenterX + circleRadius + padding;

                        canvas->drawString(radio.label.text.c_str(), textStartX, baseline, font, textPaint);
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processCheckBoxData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                 const RawCheckBoxRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    checkbox = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = checkbox.rect;
                    worldRect.x = trans.position.x - checkbox.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - checkbox.rect.Height() * 0.5f;

                    const float boxSize = std::min(worldRect.Height(), worldRect.Width() * 0.35f);
                    const float padding = 6.0f;
                    const float boxX = worldRect.x + padding;
                    const float boxY = worldRect.y + (worldRect.Height() - boxSize) * 0.5f;

                    ECS::Color baseColor = checkbox.normalColor;
                    if (checkbox.currentState == ECS::ButtonState::Hovered) baseColor = checkbox.hoverColor;
                    if (checkbox.currentState == ECS::ButtonState::Disabled) baseColor = checkbox.disabledColor;
                    if (checkbox.isChecked) baseColor = checkbox.checkedColor;
                    if (checkbox.isIndeterminate) baseColor = checkbox.indeterminateColor;

                    SkRect boxRect = SkRect::MakeXYWH(boxX, boxY, boxSize, boxSize);
                    SkPaint paint;
                    paint.setAntiAlias(true);

                    if (checkbox.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(baseColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeRectXY(boxRect, checkbox.roundness, checkbox.roundness), true);
                        canvas->drawImageRect(checkbox.backgroundImage, boxRect,
                                              SkSamplingOptions(SkFilterMode::kLinear));
                        canvas->restore();
                    }
                    else
                    {
                        paint.setColor4f({baseColor.r, baseColor.g, baseColor.b, baseColor.a});
                        canvas->drawRRect(SkRRect::MakeRectXY(boxRect, checkbox.roundness, checkbox.roundness), paint);
                    }

                    if (checkbox.isChecked)
                    {
                        if (checkbox.checkmarkImage)
                        {
                            const float inset = boxSize * 0.15f;
                            SkRect markRect = SkRect::MakeXYWH(boxX + inset, boxY + inset,
                                                               boxSize - inset * 2.0f, boxSize - inset * 2.0f);
                            canvas->drawImageRect(checkbox.checkmarkImage, markRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear));
                        }
                        else
                        {
                            SkPaint checkPaint;
                            checkPaint.setAntiAlias(true);
                            checkPaint.setStyle(SkPaint::kStroke_Style);
                            checkPaint.setStrokeWidth(2.5f);
                            checkPaint.setColor4f({
                                checkbox.checkmarkColor.r, checkbox.checkmarkColor.g,
                                checkbox.checkmarkColor.b, checkbox.checkmarkColor.a
                            });

                            SkPath path;
                            path.moveTo(boxX + boxSize * 0.2f, boxY + boxSize * 0.55f);
                            path.lineTo(boxX + boxSize * 0.43f, boxY + boxSize * 0.78f);
                            path.lineTo(boxX + boxSize * 0.8f, boxY + boxSize * 0.25f);

                            canvas->drawPath(path, checkPaint);
                        }
                    }
                    else if (checkbox.isIndeterminate)
                    {
                        SkRect barRect = SkRect::MakeXYWH(boxX + boxSize * 0.2f,
                                                          boxY + boxSize * 0.45f,
                                                          boxSize * 0.6f,
                                                          boxSize * 0.1f);
                        SkPaint barPaint;
                        barPaint.setAntiAlias(true);
                        barPaint.setColor4f({
                            checkbox.checkmarkColor.r, checkbox.checkmarkColor.g,
                            checkbox.checkmarkColor.b, checkbox.checkmarkColor.a
                        });
                        canvas->drawRoundRect(barRect, boxSize * 0.05f, boxSize * 0.05f, barPaint);
                    }

                    if (checkbox.label.typeface)
                    {
                        SkFont font(checkbox.label.typeface, checkbox.label.fontSize);
                        font.setEdging(SkFont::Edging::kAntiAlias);

                        SkPaint textPaint;
                        textPaint.setAntiAlias(true);
                        textPaint.setColor4f({
                            checkbox.label.color.r, checkbox.label.color.g,
                            checkbox.label.color.b, checkbox.label.color.a
                        });

                        SkFontMetrics metrics;
                        font.getMetrics(&metrics);
                        const float baseline = boxY + boxSize * 0.5f - (metrics.fAscent + metrics.fDescent) * 0.5f;
                        const float textX = boxX + boxSize + padding;

                        canvas->drawString(checkbox.label.text.c_str(), textX, baseline, font, textPaint);
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processSliderData(const Renderable* currIt, const ECS::TransformComponent& transform,
                               const RawSliderRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    slider = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = slider.rect;
                    worldRect.x = trans.position.x - slider.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - slider.rect.Height() * 0.5f;

                    const bool enabled = slider.isInteractable;
                    const ECS::Color trackColor = enabled ? slider.trackColor : slider.disabledColor;
                    const ECS::Color fillColor = enabled ? slider.fillColor : slider.disabledColor;
                    const ECS::Color thumbColor = enabled ? slider.thumbColor : slider.disabledColor;

                    SkPaint paint;
                    paint.setAntiAlias(true);

                    if (!slider.isVertical)
                    {
                        const float trackHeight = std::max(2.5f, worldRect.Height() * 0.25f);
                        const float trackY = worldRect.y + (worldRect.Height() - trackHeight) * 0.5f;

                        SkRect trackRect = SkRect::MakeXYWH(worldRect.x, trackY, worldRect.Width(), trackHeight);
                        if (slider.trackImage)
                        {
                            paint.setColorFilter(SkColorFilters::Blend(trackColor, SkBlendMode::kModulate));
                            canvas->drawImageRect(slider.trackImage, trackRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        }
                        else
                        {
                            paint.setColor4f({trackColor.r, trackColor.g, trackColor.b, trackColor.a});
                            canvas->drawRoundRect(trackRect, trackHeight * 0.5f, trackHeight * 0.5f, paint);
                        }

                        const float fillWidth = std::clamp(slider.normalizedValue, 0.0f, 1.0f) * worldRect.Width();
                        if (fillWidth > 0.0f)
                        {
                            SkRect fillRect = SkRect::MakeXYWH(worldRect.x, trackY, fillWidth, trackHeight);
                            if (slider.fillImage)
                            {
                                paint.setColorFilter(SkColorFilters::Blend(fillColor, SkBlendMode::kModulate));
                                canvas->drawImageRect(slider.fillImage, fillRect,
                                                      SkSamplingOptions(SkFilterMode::kLinear), &paint);
                            }
                            else
                            {
                                paint.setColor4f({fillColor.r, fillColor.g, fillColor.b, fillColor.a});
                                canvas->drawRoundRect(fillRect, trackHeight * 0.5f, trackHeight * 0.5f, paint);
                            }
                        }

                        const float thumbRadius = std::max(worldRect.Height(), trackHeight) * 0.35f;
                        const float thumbCenterX = worldRect.x + fillWidth;
                        const float thumbCenterY = worldRect.y + worldRect.Height() * 0.5f;

                        if (slider.thumbImage)
                        {
                            const float thumbSize = thumbRadius * 2.0f;
                            SkRect thumbRect = SkRect::MakeXYWH(thumbCenterX - thumbRadius, thumbCenterY - thumbRadius,
                                                                thumbSize, thumbSize);
                            paint.setColorFilter(SkColorFilters::Blend(thumbColor, SkBlendMode::kModulate));
                            canvas->drawImageRect(slider.thumbImage, thumbRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        }
                        else
                        {
                            paint.setColor4f({
                                thumbColor.r, thumbColor.g, thumbColor.b,
                                slider.isDragging ? std::min(1.0f, thumbColor.a + 0.2f) : thumbColor.a
                            });
                            canvas->drawCircle(thumbCenterX, thumbCenterY, thumbRadius, paint);
                        }
                    }
                    else
                    {
                        const float trackWidth = std::max(2.5f, worldRect.Width() * 0.25f);
                        const float trackX = worldRect.x + (worldRect.Width() - trackWidth) * 0.5f;

                        SkRect trackRect = SkRect::MakeXYWH(trackX, worldRect.y, trackWidth, worldRect.Height());
                        if (slider.trackImage)
                        {
                            paint.setColorFilter(SkColorFilters::Blend(trackColor, SkBlendMode::kModulate));
                            canvas->drawImageRect(slider.trackImage, trackRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        }
                        else
                        {
                            paint.setColor4f({trackColor.r, trackColor.g, trackColor.b, trackColor.a});
                            canvas->drawRoundRect(trackRect, trackWidth * 0.5f, trackWidth * 0.5f, paint);
                        }

                        const float fillHeight = std::clamp(slider.normalizedValue, 0.0f, 1.0f) * worldRect.Height();
                        if (fillHeight > 0.0f)
                        {
                            SkRect fillRect = SkRect::MakeXYWH(trackX, worldRect.y + worldRect.Height() - fillHeight,
                                                               trackWidth, fillHeight);
                            if (slider.fillImage)
                            {
                                paint.setColorFilter(SkColorFilters::Blend(fillColor, SkBlendMode::kModulate));
                                canvas->drawImageRect(slider.fillImage, fillRect,
                                                      SkSamplingOptions(SkFilterMode::kLinear), &paint);
                            }
                            else
                            {
                                paint.setColor4f({fillColor.r, fillColor.g, fillColor.b, fillColor.a});
                                canvas->drawRoundRect(fillRect, trackWidth * 0.5f, trackWidth * 0.5f, paint);
                            }
                        }

                        const float thumbRadius = std::max(worldRect.Width(), trackWidth) * 0.35f;
                        const float thumbCenterX = worldRect.x + worldRect.Width() * 0.5f;
                        const float thumbCenterY = worldRect.y + worldRect.Height() - fillHeight;

                        if (slider.thumbImage)
                        {
                            const float thumbSize = thumbRadius * 2.0f;
                            SkRect thumbRect = SkRect::MakeXYWH(thumbCenterX - thumbRadius, thumbCenterY - thumbRadius,
                                                                thumbSize, thumbSize);
                            paint.setColorFilter(SkColorFilters::Blend(thumbColor, SkBlendMode::kModulate));
                            canvas->drawImageRect(slider.thumbImage, thumbRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        }
                        else
                        {
                            paint.setColor4f({
                                thumbColor.r, thumbColor.g, thumbColor.b,
                                slider.isDragging ? std::min(1.0f, thumbColor.a + 0.2f) : thumbColor.a
                            });
                            canvas->drawCircle(thumbCenterX, thumbCenterY, thumbRadius, paint);
                        }
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processComboBoxData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                 const RawComboBoxRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    combo = data
                ](SkCanvas* canvas)
                {
                    if (!canvas || !combo.displayText.typeface) return;

                    ECS::RectF worldRect = combo.rect;
                    worldRect.x = trans.position.x - combo.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - combo.rect.Height() * 0.5f;

                    ECS::Color baseColor = combo.normalColor;
                    switch (combo.currentState)
                    {
                    case ECS::ButtonState::Hovered: baseColor = combo.hoverColor;
                        break;
                    case ECS::ButtonState::Pressed: baseColor = combo.pressedColor;
                        break;
                    case ECS::ButtonState::Disabled: baseColor = combo.disabledColor;
                        break;
                    case ECS::ButtonState::Normal:
                    default: baseColor = combo.normalColor;
                        break;
                    }

                    SkPaint paint;
                    paint.setAntiAlias(true);

                    SkRect boxRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), worldRect.Height());
                    if (combo.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(baseColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeRectXY(boxRect, combo.roundness, combo.roundness), true);
                        canvas->drawImageRect(combo.backgroundImage, boxRect, SkSamplingOptions(SkFilterMode::kLinear));
                        canvas->restore();
                    }
                    else
                    {
                        paint.setColor4f({baseColor.r, baseColor.g, baseColor.b, baseColor.a});
                        canvas->drawRRect(SkRRect::MakeRectXY(boxRect, combo.roundness, combo.roundness), paint);
                    }

                    SkFont font(combo.displayText.typeface, combo.displayText.fontSize);
                    font.setEdging(SkFont::Edging::kAntiAlias);

                    SkPaint textPaint;
                    textPaint.setAntiAlias(true);
                    textPaint.setColor4f({
                        combo.displayText.color.r, combo.displayText.color.g,
                        combo.displayText.color.b, combo.displayText.color.a
                    });

                    SkFontMetrics metrics;
                    font.getMetrics(&metrics);
                    const float baseline = worldRect.y + worldRect.Height() * 0.5f - (metrics.fAscent + metrics.
                        fDescent) * 0.5f;
                    const float contentPadding = 8.0f;
                    const float iconSize = std::min(worldRect.Height() * 0.5f, 18.0f);
                    const float iconX = worldRect.x + worldRect.Width() - contentPadding - iconSize;
                    const float iconY = worldRect.y + (worldRect.Height() - iconSize) * 0.5f;

                    std::string display = combo.displayText.text;
                    if (display.empty() && combo.selectedIndex >= 0 && combo.selectedIndex < static_cast<int>(combo.
                        items.size()))
                    {
                        display = combo.items[combo.selectedIndex];
                    }
                    canvas->drawString(display.c_str(), worldRect.x + contentPadding, baseline, font, textPaint);

                    if (combo.dropdownIcon)
                    {
                        SkRect iconRect = SkRect::MakeXYWH(iconX, iconY, iconSize, iconSize);
                        paint.setColorFilter(nullptr);
                        canvas->drawImageRect(combo.dropdownIcon, iconRect, SkSamplingOptions(SkFilterMode::kLinear));
                    }
                    else
                    {
                        SkPath triangle;
                        const float midX = iconX + iconSize * 0.5f;
                        const float topY = iconY + iconSize * 0.3f;
                        const float bottomY = iconY + iconSize * 0.7f;
                        triangle.moveTo(midX - iconSize * 0.25f, topY);
                        triangle.lineTo(midX + iconSize * 0.25f, topY);
                        triangle.lineTo(midX, bottomY);
                        triangle.close();

                        paint.setColor4f({
                            combo.displayText.color.r, combo.displayText.color.g,
                            combo.displayText.color.b, combo.displayText.color.a
                        });
                        canvas->drawPath(triangle, paint);
                    }

                    if (combo.isDropdownOpen && !combo.items.empty())
                    {
                        const float itemHeight = combo.displayText.fontSize * 1.4f + 6.0f;
                        const float dropdownHeight = itemHeight * static_cast<float>(combo.items.size());
                        SkRect dropdownRect = SkRect::MakeXYWH(worldRect.x,
                                                               worldRect.y + worldRect.Height(),
                                                               worldRect.Width(),
                                                               dropdownHeight);

                        SkPaint dropdownPaint;
                        dropdownPaint.setAntiAlias(true);
                        dropdownPaint.setColor4f({
                            combo.dropdownBackgroundColor.r, combo.dropdownBackgroundColor.g,
                            combo.dropdownBackgroundColor.b, combo.dropdownBackgroundColor.a
                        });
                        canvas->drawRect(dropdownRect, dropdownPaint);

                        canvas->save();
                        canvas->clipRect(dropdownRect);

                        for (size_t i = 0; i < combo.items.size(); ++i)
                        {
                            const float itemTop = dropdownRect.top() + itemHeight * static_cast<float>(i);
                            SkRect itemRect = SkRect::MakeXYWH(dropdownRect.left(), itemTop, dropdownRect.width(),
                                                               itemHeight);

                            if (static_cast<int>(i) == combo.hoveredIndex)
                            {
                                SkPaint hoverPaint;
                                hoverPaint.setAntiAlias(true);
                                hoverPaint.setColor4f({
                                    combo.hoverColor.r, combo.hoverColor.g, combo.hoverColor.b, 0.65f
                                });
                                canvas->drawRect(itemRect, hoverPaint);
                            }
                            else if (static_cast<int>(i) == combo.selectedIndex)
                            {
                                SkPaint selectedPaint;
                                selectedPaint.setAntiAlias(true);
                                selectedPaint.setColor4f({
                                    combo.pressedColor.r, combo.pressedColor.g, combo.pressedColor.b, 0.55f
                                });
                                canvas->drawRect(itemRect, selectedPaint);
                            }

                            canvas->drawString(combo.items[i].c_str(),
                                               itemRect.left() + contentPadding,
                                               itemRect.top() + itemHeight * 0.5f - (metrics.fAscent + metrics.fDescent)
                                               * 0.5f,
                                               font,
                                               textPaint);
                        }
                        canvas->restore();
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processExpanderData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                 const RawExpanderRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    expander = data
                ](SkCanvas* canvas)
                {
                    if (!canvas || !expander.header.typeface) return;

                    ECS::RectF worldRect = expander.rect;
                    worldRect.x = trans.position.x - expander.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - expander.rect.Height() * 0.5f;

                    const float headerHeight = std::min(worldRect.Height(),
                                                        std::max(28.0f, expander.header.fontSize * 1.8f));
                    SkRect headerRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), headerHeight);

                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setColor4f({
                        expander.headerColor.r, expander.headerColor.g,
                        expander.headerColor.b, expander.headerColor.a
                    });

                    canvas->drawRect(headerRect, paint);

                    if (expander.isExpanded)
                    {
                        SkRect bodyRect = SkRect::MakeXYWH(worldRect.x,
                                                           worldRect.y + headerHeight,
                                                           worldRect.Width(),
                                                           std::max(0.0f, worldRect.Height() - headerHeight));
                        paint.setColor4f({
                            expander.expandedColor.r, expander.expandedColor.g,
                            expander.expandedColor.b, expander.expandedColor.a
                        });
                        canvas->drawRect(bodyRect, paint);
                    }
                    else
                    {
                        SkRect bodyRect = SkRect::MakeXYWH(worldRect.x,
                                                           worldRect.y + headerHeight,
                                                           worldRect.Width(),
                                                           std::max(0.0f, worldRect.Height() - headerHeight));
                        paint.setColor4f({
                            expander.collapsedColor.r, expander.collapsedColor.g,
                            expander.collapsedColor.b, expander.collapsedColor.a
                        });
                        canvas->drawRect(bodyRect, paint);
                    }

                    SkFont font(expander.header.typeface, expander.header.fontSize);
                    font.setEdging(SkFont::Edging::kAntiAlias);
                    SkPaint textPaint;
                    textPaint.setAntiAlias(true);
                    textPaint.setColor4f({
                        expander.header.color.r, expander.header.color.g,
                        expander.header.color.b, expander.header.color.a
                    });

                    SkFontMetrics metrics;
                    font.getMetrics(&metrics);
                    const float baseline = headerRect.top() + headerRect.height() * 0.5f - (metrics.fAscent + metrics.
                        fDescent) * 0.5f;
                    const float padding = 10.0f;
                    const float indicatorSize = headerRect.height() * 0.35f;

                    SkPath indicator;
                    const float indicatorX = headerRect.left() + padding;
                    const float indicatorY = headerRect.top() + headerRect.height() * 0.5f;
                    if (expander.isExpanded)
                    {
                        indicator.moveTo(indicatorX - indicatorSize * 0.5f, indicatorY - indicatorSize * 0.3f);
                        indicator.lineTo(indicatorX + indicatorSize * 0.5f, indicatorY - indicatorSize * 0.3f);
                        indicator.lineTo(indicatorX, indicatorY + indicatorSize * 0.4f);
                    }
                    else
                    {
                        indicator.moveTo(indicatorX - indicatorSize * 0.3f, indicatorY - indicatorSize * 0.5f);
                        indicator.lineTo(indicatorX + indicatorSize * 0.4f, indicatorY);
                        indicator.lineTo(indicatorX - indicatorSize * 0.3f, indicatorY + indicatorSize * 0.5f);
                    }
                    indicator.close();

                    SkPaint indicatorPaint;
                    indicatorPaint.setAntiAlias(true);
                    indicatorPaint.setColor4f({
                        expander.header.color.r, expander.header.color.g,
                        expander.header.color.b, expander.header.color.a
                    });
                    canvas->drawPath(indicator, indicatorPaint);

                    canvas->drawString(expander.header.text.c_str(),
                                       headerRect.left() + padding * 2.5f,
                                       baseline,
                                       font,
                                       textPaint);
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processProgressBarData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                    const RawProgressBarRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    progress = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = progress.rect;
                    worldRect.x = trans.position.x - progress.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - progress.rect.Height() * 0.5f;

                    SkRect barRect = SkRect::MakeXYWH(worldRect.x, worldRect.y,
                                                      std::max(0.0f, worldRect.Width()),
                                                      std::max(0.0f, worldRect.Height()));

                    SkPaint paint;
                    paint.setAntiAlias(true);

                    if (progress.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(progress.backgroundColor, SkBlendMode::kModulate));
                        canvas->drawImageRect(progress.backgroundImage, barRect,
                                              SkSamplingOptions(SkFilterMode::kLinear), &paint);
                    }
                    else
                    {
                        paint.setColor4f({
                            progress.backgroundColor.r, progress.backgroundColor.g,
                            progress.backgroundColor.b, progress.backgroundColor.a
                        });
                        canvas->drawRect(barRect, paint);
                    }

                    const float range = progress.maxValue - progress.minValue;
                    float normalized = range > 1e-5f
                                           ? std::clamp((progress.value - progress.minValue) / range, 0.0f, 1.0f)
                                           : 0.0f;

                    if (progress.isIndeterminate)
                    {
                        const float segmentWidth = barRect.width() * 0.35f;
                        float start = barRect.left() + (barRect.width() + segmentWidth) * progress.indeterminatePhase -
                            segmentWidth;
                        float end = start + segmentWidth;
                        start = std::clamp(start, barRect.left(), barRect.right());
                        end = std::clamp(end, barRect.left(), barRect.right());

                        if (end > start)
                        {
                            SkRect fillRect = SkRect::MakeLTRB(start, barRect.top(), end, barRect.bottom());
                            if (progress.fillImage)
                            {
                                paint.setColorFilter(SkColorFilters::Blend(progress.fillColor, SkBlendMode::kModulate));
                                canvas->drawImageRect(progress.fillImage, fillRect,
                                                      SkSamplingOptions(SkFilterMode::kLinear), &paint);
                            }
                            else
                            {
                                paint.setColor4f({
                                    progress.fillColor.r, progress.fillColor.g,
                                    progress.fillColor.b, progress.fillColor.a
                                });
                                canvas->drawRect(fillRect, paint);
                            }
                        }
                    }
                    else if (normalized > 0.0f)
                    {
                        SkRect fillRect = SkRect::MakeXYWH(barRect.left(),
                                                           barRect.top(),
                                                           barRect.width() * normalized,
                                                           barRect.height());
                        if (progress.fillImage)
                        {
                            paint.setColorFilter(SkColorFilters::Blend(progress.fillColor, SkBlendMode::kModulate));
                            canvas->drawImageRect(progress.fillImage, fillRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        }
                        else
                        {
                            paint.setColor4f({
                                progress.fillColor.r, progress.fillColor.g,
                                progress.fillColor.b, progress.fillColor.a
                            });
                            canvas->drawRect(fillRect, paint);
                        }
                    }

                    paint.setColorFilter(nullptr);
                    paint.setStyle(SkPaint::kStroke_Style);
                    paint.setStrokeWidth(1.5f);
                    paint.setColor4f({
                        progress.borderColor.r, progress.borderColor.g,
                        progress.borderColor.b, progress.borderColor.a
                    });
                    canvas->drawRect(barRect, paint);
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processTabControlData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                   const RawTabControlRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    tabs = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = tabs.rect;
                    worldRect.x = trans.position.x - tabs.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - tabs.rect.Height() * 0.5f;

                    SkPaint paint;
                    paint.setAntiAlias(true);
                    paint.setColor4f({
                        tabs.backgroundColor.r, tabs.backgroundColor.g,
                        tabs.backgroundColor.b, tabs.backgroundColor.a
                    });
                    SkRect fullRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), worldRect.Height());
                    if (tabs.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(tabs.backgroundColor, SkBlendMode::kModulate));
                        canvas->drawImageRect(tabs.backgroundImage, fullRect, SkSamplingOptions(SkFilterMode::kLinear),
                                              &paint);
                    }
                    else
                    {
                        canvas->drawRect(fullRect, paint);
                    }

                    const float headerHeight = tabs.tabHeight;
                    float cursor = worldRect.x + 4.0f;

                    SkFont font(nullptr, headerHeight * 0.42f);
                    font.setEdging(SkFont::Edging::kAntiAlias);
                    SkPaint textPaint;
                    textPaint.setAntiAlias(true);
                    textPaint.setColor(SK_ColorWHITE);

                    for (size_t i = 0; i < tabs.tabs.size(); ++i)
                    {
                        const auto& tabItem = tabs.tabs[i];
                        if (!tabItem.isVisible) continue;

                        const float titleFactor = static_cast<float>(tabItem.title.size()) * 0.6f + 2.0f;
                        const float tabWidth = std::clamp(headerHeight * titleFactor, headerHeight * 1.8f,
                                                          worldRect.Width());
                        SkRect tabRect = SkRect::MakeXYWH(cursor, worldRect.y, tabWidth, headerHeight);

                        ECS::Color tabColor = tabs.tabColor;
                        if (!tabItem.isEnabled)
                        {
                            tabColor = tabs.disabledTabColor;
                        }
                        else if (static_cast<int>(i) == tabs.activeTabIndex)
                        {
                            tabColor = tabs.activeTabColor;
                        }
                        else if (static_cast<int>(i) == tabs.hoveredTabIndex)
                        {
                            tabColor = tabs.hoverTabColor;
                        }

                        if (tabs.tabBackgroundImage)
                        {
                            paint.setColorFilter(SkColorFilters::Blend(tabColor, SkBlendMode::kModulate));
                            canvas->drawImageRect(tabs.tabBackgroundImage, tabRect,
                                                  SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        }
                        else
                        {
                            paint.setColorFilter(nullptr);
                            paint.setColor4f({tabColor.r, tabColor.g, tabColor.b, tabColor.a});
                            canvas->drawRect(tabRect, paint);
                        }

                        SkPaint borderPaint;
                        borderPaint.setAntiAlias(true);
                        borderPaint.setStyle(SkPaint::kStroke_Style);
                        borderPaint.setStrokeWidth(1.0f);
                        borderPaint.setColor4f({
                            tabs.backgroundColor.r * 0.5f,
                            tabs.backgroundColor.g * 0.5f,
                            tabs.backgroundColor.b * 0.5f,
                            1.0f
                        });
                        canvas->drawRect(tabRect, borderPaint);

                        SkFontMetrics metrics;
                        font.getMetrics(&metrics);
                        const float baseline = tabRect.top() + tabRect.height() * 0.5f - (metrics.fAscent + metrics.
                            fDescent) * 0.5f;
                        canvas->drawString(tabItem.title.c_str(), tabRect.left() + 10.0f, baseline, font, textPaint);

                        cursor += tabWidth + tabs.tabSpacing;
                    }
                }
            );

            result->rawDrawPackets.emplace_back(RenderPacket{
                .zIndex = batch.zIndex,
                .batchData = std::move(batch)
            });
        }

        void processListBoxData(const Renderable* currIt, const ECS::TransformComponent& transform,
                                const RawListBoxRenderData& data)
        {
            RawDrawBatch batch;
            batch.zIndex = currIt->zIndex;

            batch.drawFunc.AddListener(
                [
                    trans = transform,
                    listBox = data
                ](SkCanvas* canvas)
                {
                    if (!canvas) return;

                    ECS::RectF worldRect = listBox.rect;
                    worldRect.x = trans.position.x - listBox.rect.Width() * 0.5f;
                    worldRect.y = trans.position.y - listBox.rect.Height() * 0.5f;

                    SkRect boxRect = SkRect::MakeXYWH(worldRect.x, worldRect.y, worldRect.Width(), worldRect.Height());

                    SkPaint paint;
                    paint.setAntiAlias(true);
                    if (listBox.backgroundImage)
                    {
                        paint.setColorFilter(SkColorFilters::Blend(listBox.backgroundColor, SkBlendMode::kModulate));
                        canvas->save();
                        canvas->clipRRect(SkRRect::MakeRectXY(boxRect, listBox.roundness, listBox.roundness), true);
                        canvas->drawImageRect(listBox.backgroundImage, boxRect,
                                              SkSamplingOptions(SkFilterMode::kLinear), &paint);
                        canvas->restore();
                    }
                    else
                    {
                        paint.setColor4f({
                            listBox.backgroundColor.r, listBox.backgroundColor.g,
                            listBox.backgroundColor.b, listBox.backgroundColor.a
                        });
                        canvas->drawRRect(SkRRect::MakeRectXY(boxRect, listBox.roundness, listBox.roundness), paint);
                    }

                    const int itemCount = std::max(0, listBox.itemCount);
                    const float spacingX = std::max(0.0f, listBox.itemSpacing.x);
                    const float spacingY = std::max(0.0f, listBox.itemSpacing.y);
                    const float trackSpacing = 4.0f;
                    const float trackThickness = std::max(listBox.scrollbarThickness, 2.0f);

                    auto clampPositive = [](int value, int fallback)
                    {
                        return value > 0 ? value : fallback;
                    };

                    const int visibleCandidate = listBox.visibleItemCount > 0
                                                     ? std::min(listBox.visibleItemCount, std::max(1, itemCount))
                                                     : std::max(1, itemCount);

                    int columns = 1;
                    int rows = 1;
                    int itemsPerPage = 0;

                    switch (listBox.layout)
                    {
                    case ECS::ListBoxLayout::Horizontal:
                        rows = clampPositive(listBox.maxItemsPerColumn, 1);
                        if (listBox.visibleItemCount > 0)
                        {
                            columns = std::max(
                                1, static_cast<int>(std::ceil(
                                    static_cast<float>(visibleCandidate) / static_cast<float>(rows))));
                            itemsPerPage = std::min(itemCount, columns * rows);
                        }
                        else
                        {
                            columns = std::max(1, static_cast<int>(std::ceil(
                                                   static_cast<float>(std::max(1, itemCount)) / static_cast<float>(
                                                       rows))));
                            itemsPerPage = itemCount;
                        }
                        break;
                    case ECS::ListBoxLayout::Grid:
                        columns = clampPositive(listBox.maxItemsPerRow, 1);
                        rows = clampPositive(listBox.maxItemsPerColumn, 1);
                        if (listBox.visibleItemCount > 0)
                        {
                            int target = std::min(visibleCandidate, std::max(1, itemCount));
                            if (listBox.maxItemsPerRow <= 0 && listBox.maxItemsPerColumn <= 0)
                            {
                                columns = std::max(
                                    1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(target)))));
                                rows = std::max(
                                    1, static_cast<int>(std::ceil(
                                        static_cast<float>(target) / static_cast<float>(columns))));
                            }
                            else if (listBox.maxItemsPerRow <= 0)
                            {
                                rows = clampPositive(listBox.maxItemsPerColumn, 1);
                                columns = std::max(
                                    1, static_cast<int>(
                                        std::ceil(static_cast<float>(target) / static_cast<float>(rows))));
                            }
                            else if (listBox.maxItemsPerColumn <= 0)
                            {
                                columns = std::max(1, std::min(listBox.maxItemsPerRow, target));
                                rows = std::max(
                                    1, static_cast<int>(std::ceil(
                                        static_cast<float>(target) / static_cast<float>(columns))));
                            }
                            itemsPerPage = std::min(itemCount, columns * rows);
                        }
                        else
                        {
                            itemsPerPage = std::min(itemCount, columns * rows);
                        }
                        break;
                    case ECS::ListBoxLayout::Vertical:
                    default:
                        columns = clampPositive(listBox.maxItemsPerRow, 1);
                        if (listBox.visibleItemCount > 0)
                        {
                            rows = std::max(
                                1, static_cast<int>(std::ceil(
                                    static_cast<float>(visibleCandidate) / static_cast<float>(columns))));
                            itemsPerPage = std::min(itemCount, rows * columns);
                        }
                        else
                        {
                            rows = std::max(1, static_cast<int>(std::ceil(
                                                static_cast<float>(std::max(1, itemCount)) / static_cast<float>(
                                                    columns))));
                            itemsPerPage = itemCount;
                        }
                        break;
                    }


                    if (listBox.visibleItemCount <= 0)
                    {
                        const float approxLineHeight = listBox.itemTemplate.fontSize > 0.0f
                                                           ? (listBox.itemTemplate.fontSize * 1.4f)
                                                           : 20.0f;
                        const float containerWidth = listBox.rect.Width();
                        const float containerHeight = listBox.rect.Height();

                        switch (listBox.layout)
                        {
                        case ECS::ListBoxLayout::Horizontal:
                            {
                                int maxTextLen = 1;
                                for (const auto& s : listBox.items)
                                    maxTextLen = std::max<int>(maxTextLen, static_cast<int>(s.size()));
                                const float estCharWidth = std::max(1.0f, listBox.itemTemplate.fontSize * 0.6f);
                                const float paddingX = 8.0f;
                                const float estItemWidth = paddingX * 2.0f + estCharWidth * static_cast<float>(
                                    maxTextLen);

                                rows = clampPositive(listBox.maxItemsPerColumn, 1);
                                columns = std::max(
                                    1, static_cast<int>(std::floor(
                                        (containerWidth + spacingX) / (estItemWidth + spacingX))));
                                itemsPerPage = std::min(itemCount, rows * columns);
                                break;
                            }
                        case ECS::ListBoxLayout::Grid:
                        case ECS::ListBoxLayout::Vertical:
                        default:
                            {
                                columns = clampPositive(listBox.maxItemsPerRow, 1);
                                const float cellH = approxLineHeight;

                                rows = std::max(
                                    1, static_cast<int>(std::floor((containerHeight + spacingY) / (cellH + spacingY))));
                                itemsPerPage = std::min(itemCount, rows * columns);
                                break;
                            }
                        }
                    }

                    if (itemCount == 0)
                    {
                        itemsPerPage = 0;
                    }
                    else
                    {
                        itemsPerPage = std::max(1, itemsPerPage);
                    }

                    const bool primaryIsVertical = listBox.layout != ECS::ListBoxLayout::Horizontal;
                    bool verticalScrollable = primaryIsVertical && itemCount > itemsPerPage;
                    bool baseHorizontalScrollable = !primaryIsVertical && itemCount > itemsPerPage;
                    bool showVertical = listBox.enableVerticalScrollbar && (verticalScrollable || !listBox.
                        verticalScrollbarAutoHide);

                    float contentLeft = boxRect.left() + 2.0f;
                    float contentRight = boxRect.right() - 2.0f - (
                        showVertical ? (trackThickness + trackSpacing) : 0.0f);
                    float contentTop = boxRect.top() + 2.0f;
                    float contentBottom = boxRect.bottom() - 2.0f;

                    const bool drawText = !listBox.useContainer && listBox.itemTemplate.typeface;
                    SkFont font;
                    SkFontMetrics metrics{};
                    SkPaint textPaint;
                    if (drawText)
                    {
                        font = SkFont(listBox.itemTemplate.typeface, listBox.itemTemplate.fontSize);
                        font.setEdging(SkFont::Edging::kAntiAlias);
                        font.getMetrics(&metrics);

                        textPaint.setAntiAlias(true);
                        textPaint.setColor4f({
                            listBox.itemColor.r, listBox.itemColor.g,
                            listBox.itemColor.b, listBox.itemColor.a
                        });
                    }

                    float availableWidth = std::max(0.0f, contentRight - contentLeft);
                    float availableHeight = std::max(0.0f, contentBottom - contentTop);

                    float maxContentWidth = drawText ? 0.0f : availableWidth;
                    if (drawText)
                    {
                        for (const auto& text : listBox.items)
                        {
                            SkRect bounds{};
                            font.measureText(text.c_str(), text.size(), SkTextEncoding::kUTF8, &bounds);
                            maxContentWidth = std::max(maxContentWidth, bounds.width() + 16.0f);
                        }
                    }

                    bool textHorizontalScrollable = drawText && maxContentWidth > availableWidth;
                    bool showHorizontal = listBox.enableHorizontalScrollbar &&
                    ((baseHorizontalScrollable || textHorizontalScrollable) || !listBox.
                        horizontalScrollbarAutoHide);

                    if (showHorizontal)
                    {
                        contentBottom -= (trackThickness + trackSpacing);
                        availableHeight = std::max(0.0f, contentBottom - contentTop);
                    }

                    int maxScroll = std::max(0, itemCount - itemsPerPage);
                    int startIndex = std::clamp(listBox.scrollOffset, 0, maxScroll);
                    int endIndex = (itemsPerPage > 0) ? std::min(itemCount, startIndex + itemsPerPage) : 0;

                    columns = std::max(1, columns);
                    rows = std::max(1, rows);


                    float itemWidth = 0.0f;
                    float itemHeight = 0.0f;

                    if (drawText)
                    {
                        const float textHeight = metrics.fDescent - metrics.fAscent;
                        const float paddingY = 8.0f;
                        const float actualItemHeight = textHeight + paddingY * 2.0f;


                        float totalSpacingX = spacingX * static_cast<float>(std::max(0, columns - 1));
                        float totalSpacingY = spacingY * static_cast<float>(std::max(0, rows - 1));

                        itemWidth = columns > 0
                                        ? (availableWidth - totalSpacingX) / static_cast<float>(columns)
                                        : availableWidth;


                        if (primaryIsVertical)
                        {
                            itemHeight = actualItemHeight;
                        }
                        else
                        {
                            itemHeight = rows > 0
                                             ? (availableHeight - totalSpacingY) / static_cast<float>(rows)
                                             : availableHeight;
                        }
                    }
                    else
                    {
                        float totalSpacingX = spacingX * static_cast<float>(std::max(0, columns - 1));
                        float totalSpacingY = spacingY * static_cast<float>(std::max(0, rows - 1));

                        itemWidth = columns > 0
                                        ? (availableWidth - totalSpacingX) / static_cast<float>(columns)
                                        : availableWidth;
                        itemHeight = rows > 0
                                         ? (availableHeight - totalSpacingY) / static_cast<float>(rows)
                                         : availableHeight;
                    }

                    itemWidth = std::max(itemWidth, 1.0f);
                    itemHeight = std::max(itemHeight, 1.0f);

                    const float paddingX = 8.0f;

                    SkRect contentRect = SkRect::MakeXYWH(contentLeft, contentTop, availableWidth, availableHeight);
                    if (contentRect.width() > 0.0f && contentRect.height() > 0.0f && endIndex > startIndex)
                    {
                        canvas->save();
                        canvas->clipRect(contentRect, true);

                        for (int i = startIndex; i < endIndex; ++i)
                        {
                            const int visibleIndex = i - startIndex;
                            int row = 0;
                            int column = 0;
                            if (listBox.layout == ECS::ListBoxLayout::Horizontal)
                            {
                                column = rows > 0 ? visibleIndex / rows : 0;
                                row = rows > 0 ? visibleIndex % rows : 0;
                            }
                            else
                            {
                                row = columns > 0 ? visibleIndex / columns : 0;
                                column = columns > 0 ? visibleIndex % columns : 0;
                            }

                            float x = contentLeft + static_cast<float>(column) * (itemWidth + spacingX);
                            float y = contentTop + static_cast<float>(row) * (itemHeight + spacingY);
                            SkRect itemRect = SkRect::MakeXYWH(x, y, itemWidth, itemHeight);

                            bool isSelected = std::find(listBox.selectedIndices.begin(), listBox.selectedIndices.end(),
                                                        i)
                                != listBox.selectedIndices.end();
                            bool isHovered = (i == listBox.hoveredIndex);

                            if (isSelected || isHovered)
                            {
                                ECS::Color highlightColor = isSelected ? listBox.selectedColor : listBox.hoverColor;
                                SkPaint highlightPaint;
                                highlightPaint.setAntiAlias(true);
                                highlightPaint.setColor4f({
                                    highlightColor.r, highlightColor.g,
                                    highlightColor.b, std::min(1.0f, highlightColor.a + 0.2f)
                                });
                                canvas->drawRect(itemRect, highlightPaint);
                            }

                            if (drawText && i < static_cast<int>(listBox.items.size()))
                            {
                                float baseline = itemRect.top() + itemRect.height() * 0.5f - (metrics.fAscent + metrics.
                                    fDescent) * 0.5f;
                                canvas->drawString(listBox.items[i].c_str(),
                                                   itemRect.left() + paddingX,
                                                   baseline,
                                                   font,
                                                   textPaint);
                            }
                        }

                        canvas->restore();
                    }

                    if (showVertical && availableHeight > 0.0f)
                    {
                        SkRect trackRect = SkRect::MakeXYWH(contentRight + trackSpacing,
                                                            contentTop,
                                                            trackThickness,
                                                            availableHeight);
                        SkPaint trackPaint;
                        trackPaint.setAntiAlias(true);
                        trackPaint.setColor4f({
                            listBox.scrollbarTrackColor.r, listBox.scrollbarTrackColor.g,
                            listBox.scrollbarTrackColor.b, listBox.scrollbarTrackColor.a
                        });
                        canvas->drawRRect(SkRRect::MakeRectXY(trackRect, trackThickness * 0.5f, trackThickness * 0.5f),
                                          trackPaint);

                        float thumbHeight = trackRect.height();
                        if (itemCount > 0 && itemsPerPage > 0)
                        {
                            float ratio = static_cast<float>(itemsPerPage) / static_cast<float>(itemCount);
                            thumbHeight = std::max(trackRect.height() * std::clamp(ratio, 0.0f, 1.0f), trackThickness);
                        }

                        float scrollRange = std::max(1, itemCount - itemsPerPage);
                        float thumbOffset = 0.0f;
                        if (scrollRange > 0 && itemsPerPage > 0)
                        {
                            float ratio = static_cast<float>(std::clamp(static_cast<float>(listBox.scrollOffset), 0.0f,
                                                                        static_cast<float>(scrollRange))) / static_cast<
                                float>(scrollRange);
                            thumbOffset = (trackRect.height() - thumbHeight) * std::clamp(ratio, 0.0f, 1.0f);
                        }

                        SkRect thumbRect = SkRect::MakeXYWH(trackRect.left(), trackRect.top() + thumbOffset,
                                                            trackRect.width(), thumbHeight);
                        SkPaint thumbPaint;
                        thumbPaint.setAntiAlias(true);
                        thumbPaint.setColor4f({
                            listBox.scrollbarThumbColor.r, listBox.scrollbarThumbColor.g,
                            listBox.scrollbarThumbColor.b, listBox.scrollbarThumbColor.a
                        });
                        canvas->drawRRect(SkRRect::MakeRectXY(thumbRect, trackThickness * 0.5f, trackThickness * 0.5f),
                                          thumbPaint);
                    }

                    if (showHorizontal && availableWidth > 0.0f)
                    {
                        SkRect trackRect = SkRect::MakeXYWH(contentLeft,
                                                            contentBottom + trackSpacing,
                                                            availableWidth,
                                                            trackThickness);
                        SkPaint trackPaint;
                        trackPaint.setAntiAlias(true);
                        trackPaint.setColor4f({
                            listBox.scrollbarTrackColor.r, listBox.scrollbarTrackColor.g,
                            listBox.scrollbarTrackColor.b, listBox.scrollbarTrackColor.a
                        });
                        canvas->drawRRect(SkRRect::MakeRectXY(trackRect, trackThickness * 0.5f, trackThickness * 0.5f),
                                          trackPaint);

                        float thumbWidth = trackRect.width();
                        float thumbOffset = 0.0f;
                        if (baseHorizontalScrollable && itemCount > 0 && itemsPerPage > 0)
                        {
                            float ratio = static_cast<float>(itemsPerPage) / static_cast<float>(itemCount);
                            thumbWidth = std::max(trackRect.width() * std::clamp(ratio, 0.0f, 1.0f), trackThickness);
                            float scrollRange = std::max(1, itemCount - itemsPerPage);
                            if (scrollRange > 0)
                            {
                                float offsetRatio = static_cast<float>(std::clamp(
                                    static_cast<float>(listBox.scrollOffset), 0.0f,
                                    static_cast<float>(scrollRange))) / static_cast<float>(scrollRange);
                                thumbOffset = (trackRect.width() - thumbWidth) * std::clamp(offsetRatio, 0.0f, 1.0f);
                            }
                        }
                        else if (textHorizontalScrollable && maxContentWidth > availableWidth && maxContentWidth > 0.0f)
                        {
                            float ratio = availableWidth / maxContentWidth;
                            thumbWidth = std::max(trackRect.width() * std::clamp(ratio, 0.0f, 1.0f), trackThickness);
                        }

                        SkRect thumbRect = SkRect::MakeXYWH(trackRect.left() + thumbOffset, trackRect.top(), thumbWidth,
                                                            trackRect.height());
                        SkPaint thumbPaint;
                        thumbPaint.setAntiAlias(true);
                        thumbPaint.setColor4f({
                            listBox.scrollbarThumbColor.r, listBox.scrollbarThumbColor.g,
                            listBox.scrollbarThumbColor.b, listBox.scrollbarThumbColor.a
                        });
                        canvas->drawRRect(SkRRect::MakeRectXY(thumbRect, trackThickness * 0.5f, trackThickness * 0.5f),
                                          thumbPaint);
                    }
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
                                   static_cast<ECS::WrapMode>(spriteData.wrapMode),
                                   spriteData.sourceRect, spriteData.ppuScaleFactor, currIt->zIndex);

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
                                 static_cast<TextAlignment>(textData.alignment), textData.color,
                                 currIt->zIndex);

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
            proxy.Sort([](const Renderable& a, const Renderable& b)
            {
                return static_cast<uint32_t>(a.entityId) < static_cast<uint32_t>(b.entityId);
            });
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
            proxy.Sort([](const Renderable& a, const Renderable& b)
            {
                return static_cast<uint32_t>(a.entityId) < static_cast<uint32_t>(b.entityId);
            });
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
        proxy = std::move(frameData);
    });

    SubmitFrame(std::move(dynamicFrameData));
}

const std::vector<RenderPacket>& RenderableManager::GetInterpolationData()
{
    while (isUpdatingFrames.exchange(true, std::memory_order_acquire))
    {
    }

    if (!needsRebuild())
    {
        isUpdatingFrames.store(false, std::memory_order_release);
        return packetBuffers[activeBufferIndex.load(std::memory_order_relaxed)];
    }

    auto prevFrameView = prevFrame.GetView();
    auto currFrameView = currFrame.GetView();

    isUpdatingFrames.store(false, std::memory_order_release);

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
    auto baseFrameView = currFrameView;

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
                                       static_cast<ECS::WrapMode>(threadGroup.wrapMode),
                                       threadGroup.sourceRect, threadGroup.ppuScaleFactor, threadGroup.zIndex);

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
                                     threadGroup.alignment, threadGroup.color,
                                     threadGroup.zIndex);

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
        if (a.zIndex != b.zIndex) return a.zIndex < b.zIndex;
        int ta = packet_type_index(a);
        int tb = packet_type_index(b);
        if (ta != tb) return ta < tb;
        uint64_t ka = stable_packet_key(a);
        uint64_t kb = stable_packet_key(b);
        return ka < kb;
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
