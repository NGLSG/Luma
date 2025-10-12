#include "RenderSystem.h"
#include "GraphicsBackend.h"
#include <stdexcept>
#include <unordered_map>
#include <cmath>
#include <numbers>
#include <algorithm>


#include <include/core/SkCanvas.h>
#include <include/core/SkPaint.h>
#include <include/core/SkVertices.h>
#include <include/core/SkShader.h>
#include <include/effects/SkRuntimeEffect.h>
#include <include/core/SkRSXform.h>

#include "Camera.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "Renderer/RenderComponent.h"
#include <functional>
#include <SIMDWrapper.h>

#include "Profiler.h"

static SkFilterMode GetSkFilterMode(int quality)
{
    switch (quality)
    {
    case 0: return SkFilterMode::kNearest;
    case 2: return SkFilterMode::kLinear;
    default: return SkFilterMode::kLinear;
    }
}

static SkMipmapMode GetSkMipmapMode(int quality)
{
    return (quality == 2) ? SkMipmapMode::kLinear : SkMipmapMode::kNone;
}

static SkTileMode GetSkTileMode(int wrapMode)
{
    switch (wrapMode)
    {
    case 1: return SkTileMode::kRepeat;
    case 2: return SkTileMode::kMirror;
    default: return SkTileMode::kClamp;
    }
}

struct CursorPrimitive
{
    SkPoint position;
    float height;
    SkColor4f color;
};

class RenderSystem::RenderSystemImpl
{
public:
    GraphicsBackend& backend;
    const size_t maxPrimitivesPerBatch;
    std::optional<SkRect> clipRect;

    std::vector<SpriteBatch> spriteBatches;
    std::vector<TextBatch> textBatches;
    std::vector<InstanceBatch> instanceBatches;
    std::vector<RectBatch> rectBatches;
    std::vector<CircleBatch> circleBatches;
    std::vector<LineBatch> lineBatches;
    std::vector<ShaderBatch> shaderBatches;
    std::vector<RawDrawBatch> rawDrawBatches;
    std::vector<CursorPrimitive> cursorPrimitives;

    std::vector<SkPoint> positions;
    std::vector<SkPoint> texCoords;
    std::vector<SkColor> colors;
    std::vector<uint16_t> indices;


    std::vector<SkRSXform> rsxforms;
    std::vector<SkRect> texRects;


    explicit RenderSystemImpl(GraphicsBackend& backendRef, size_t maxPrimitives)
        : backend(backendRef), maxPrimitivesPerBatch(maxPrimitives)
    {
        if (maxPrimitives == 0)
        {
            throw std::invalid_argument("maxPrimitivesPerBatch must be greater than 0.");
        }

        const size_t maxVertices = maxPrimitives * 4;
        const size_t maxIndices = maxPrimitives * 6;

        positions.reserve(maxVertices);
        texCoords.reserve(maxVertices);
        colors.reserve(maxVertices);
        indices.reserve(maxIndices);

        rsxforms.reserve(maxPrimitives);
        texRects.reserve(maxPrimitives);

        spriteBatches.reserve(32);
        textBatches.reserve(32);
        instanceBatches.reserve(32);
        rectBatches.reserve(32);
        circleBatches.reserve(32);
        lineBatches.reserve(32);
        shaderBatches.reserve(32);
        rawDrawBatches.reserve(16);
        cursorPrimitives.reserve(16);
    }


    void ClearBatches()
    {
        spriteBatches.clear();
        textBatches.clear();
        instanceBatches.clear();
        rectBatches.clear();
        circleBatches.clear();
        lineBatches.clear();
        shaderBatches.clear();
        cursorPrimitives.clear();
        rawDrawBatches.clear();
    }


    void DrawAllSpriteBatches(SkCanvas* canvas);
    void DrawAllTextBatches(SkCanvas* canvas);
    void DrawAllInstanceBatches(SkCanvas* canvas);
    void DrawAllRectBatches(SkCanvas* canvas);
    void DrawAllCircleBatches(SkCanvas* canvas);
    void DrawAllLineBatches(SkCanvas* canvas);
    void DrawAllShaderBatches(SkCanvas* canvas);
    void DrawAllCursorBatches(SkCanvas* canvas);
    void DrawAllRawDrawBatches(SkCanvas* canvas);

private:
    enum class TextAlignment
    {
        TopLeft = 0, TopCenter, TopRight,
        MiddleLeft, MiddleCenter, MiddleRight,
        BottomLeft, BottomCenter, BottomRight
    };

    static std::vector<std::string> splitStringByNewline(const std::string& str)
    {
        std::vector<std::string> lines;
        if (str.empty()) return lines;
        std::string line;
        std::istringstream stream(str);
        while (std::getline(stream, line))
        {
            lines.push_back(line);
        }
        return lines;
    }


    static SkPoint calculateTextAlignmentOffset(const std::string& line, const SkFont& font, TextAlignment alignment)
    {
        float xOffset = 0;

        switch (alignment)
        {
        case TextAlignment::TopCenter:
        case TextAlignment::MiddleCenter:
        case TextAlignment::BottomCenter:
            {
                SkRect bounds;
                font.measureText(line.c_str(), line.size(), SkTextEncoding::kUTF8, &bounds);
                xOffset = -bounds.width() / 2.0f;
                break;
            }
        case TextAlignment::TopRight:
        case TextAlignment::MiddleRight:
        case TextAlignment::BottomRight:
            {
                SkRect bounds;
                font.measureText(line.c_str(), line.size(), SkTextEncoding::kUTF8, &bounds);
                xOffset = -bounds.width();
                break;
            }
        default:
            break;
        }
        return {xOffset, 0};
    }
};


std::unique_ptr<RenderSystem> RenderSystem::Create(GraphicsBackend& backend, size_t maxPrimitivesPerBatch)
{
    return std::unique_ptr<RenderSystem>(new RenderSystem(backend, maxPrimitivesPerBatch));
}


RenderSystem::RenderSystem(GraphicsBackend& backend, size_t maxPrimitivesPerBatch)
    : pImpl(std::make_unique<RenderSystemImpl>(backend, maxPrimitivesPerBatch))
{
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::Submit(const SpriteBatch& batch) { if (batch.count > 0) pImpl->spriteBatches.push_back(batch); }

void RenderSystem::Submit(const TextBatch& batch)
{
    if (batch.count > 0) pImpl->textBatches.push_back(batch);
}

void RenderSystem::Submit(const InstanceBatch& batch) { if (batch.count > 0) pImpl->instanceBatches.push_back(batch); }
void RenderSystem::Submit(const RectBatch& batch) { if (batch.count > 0) pImpl->rectBatches.push_back(batch); }
void RenderSystem::Submit(const CircleBatch& batch) { if (batch.count > 0) pImpl->circleBatches.push_back(batch); }
void RenderSystem::Submit(const LineBatch& batch) { if (batch.pointCount >= 2) pImpl->lineBatches.push_back(batch); }

void RenderSystem::Submit(const ShaderBatch& batch)
{
    if (batch.material && batch.material->effect)
    {
        pImpl->shaderBatches.push_back(batch);
    }
}

void RenderSystem::Submit(const RawDrawBatch& batch)
{
    if (batch.drawFunc)
    {
        pImpl->rawDrawBatches.push_back(batch);
    }
}

void RenderSystem::DrawCursor(const SkPoint& position, float height, const SkColor4f& color)
{
    pImpl->cursorPrimitives.emplace_back(CursorPrimitive{position, height, color});
}

void RenderSystem::Submit(const RenderPacket& packet)
{
    PROFILE_FUNCTION();
    std::visit([this](auto&& batch)
    {
        this->Submit(std::forward<decltype(batch)>(batch));
    }, packet.batchData);
}


void RenderSystem::Clear(const SkColor4f& color)
{
    PROFILE_FUNCTION();
    auto surface = pImpl->backend.GetSurface();
    if (surface && surface->getCanvas())
    {
        surface->getCanvas()->clear(color);
    }
}


void RenderSystem::Clear(const SkColor& color)
{
    PROFILE_FUNCTION();
    auto surface = pImpl->backend.GetSurface();
    if (surface && surface->getCanvas())
    {
        surface->getCanvas()->clear(color);
    }
}


void RenderSystem::Flush()
{
    PROFILE_FUNCTION();
    auto surface = pImpl->backend.GetSurface();
    if (!surface) return;

    SkCanvas* canvas = surface->getCanvas();
    if (!canvas) return;


    bool hasClip = pImpl->clipRect.has_value();
    if (hasClip)
    {
        const SkRect& viewport = *pImpl->clipRect;
        canvas->save();
        canvas->clipRect(viewport);
        canvas->translate(viewport.fLeft, viewport.fTop);
    }
    canvas->save();

    canvas->clear(Camera::GetInstance().GetProperties().clearColor);
    Camera::GetInstance().ApplyTo(canvas);

    pImpl->DrawAllSpriteBatches(canvas);
    pImpl->DrawAllTextBatches(canvas);
    pImpl->DrawAllInstanceBatches(canvas);
    pImpl->DrawAllRectBatches(canvas);
    pImpl->DrawAllCircleBatches(canvas);
    pImpl->DrawAllLineBatches(canvas);
    pImpl->DrawAllShaderBatches(canvas);
    pImpl->DrawAllCursorBatches(canvas);
    pImpl->DrawAllRawDrawBatches(canvas);
    canvas->restore();
    if (hasClip)
    {
        canvas->restore();
    }

    pImpl->ClearBatches();
}


void RenderSystem::SetClipRect(const SkRect& rect)
{
    pImpl->clipRect = rect;
}


void RenderSystem::ClearClipRect()
{
    pImpl->clipRect.reset();
}


void RenderSystem::RenderSystemImpl::DrawAllSpriteBatches(SkCanvas* canvas)
{
    if (spriteBatches.empty())
    {
        return;
    }

    auto& simd = SIMD::GetInstance();

    SkPaint paint;
    paint.setBlendMode(SkBlendMode::kSrcOver);

    const size_t maxVerticesPerDraw = maxPrimitivesPerBatch * 4;
    const size_t maxIndicesPerDraw = maxPrimitivesPerBatch * 6;
    positions.resize(maxVerticesPerDraw);
    texCoords.resize(maxVerticesPerDraw);
    colors.resize(maxVerticesPerDraw);
    indices.resize(maxIndicesPerDraw);

    constexpr size_t simd_chunk_size = 8;
    alignas(32) float pos_x_soa[simd_chunk_size], pos_y_soa[simd_chunk_size];
    alignas(32) float scale_x_soa[simd_chunk_size], scale_y_soa[simd_chunk_size];
    alignas(32) float sin_r_soa[simd_chunk_size], cos_r_soa[simd_chunk_size];

    alignas(32) float scaled_half_w[simd_chunk_size], scaled_half_h[simd_chunk_size];
    alignas(32) float scaled_local_x[simd_chunk_size], scaled_local_y[simd_chunk_size];
    alignas(32) float final_x[simd_chunk_size], final_y[simd_chunk_size];


    for (const auto& batch : spriteBatches)
    {
        const auto* material = batch.material;
        const auto* image = batch.image.get();
        const SkColor skColor = batch.color.toSkColor();

        if (!image) continue;

        const SkTileMode tileX = GetSkTileMode(batch.wrapMode);
        const SkTileMode tileY = GetSkTileMode(batch.wrapMode);
        const SkSamplingOptions sampling(GetSkFilterMode(batch.filterQuality), GetSkMipmapMode(batch.filterQuality));

        sk_sp<SkShader> shader = nullptr;
        if (material && material->effect)
        {
            SkRuntimeShaderBuilder builder(material->effect);
            for (const auto& [name, value] : material->uniforms)
            {
                std::visit(UniformSetter{builder, name}, value);
            }
            sk_sp<SkShader> imageShader = batch.image->makeShader(tileX, tileY, sampling);
            builder.child("_MainTex") = std::move(imageShader);
            shader = builder.makeShader();
        }
        else
        {
            shader = batch.image->makeShader(tileX, tileY, sampling);
        }

        if (!shader) continue;

        paint.setShader(std::move(shader));


        size_t currentVertexCount = 0;
        size_t currentIndexCount = 0;

        auto flushDrawCall = [&]()
        {
            if (currentVertexCount == 0) return;
            sk_sp<SkVertices> vertices = SkVertices::MakeCopy(
                SkVertices::kTriangles_VertexMode, currentVertexCount,
                positions.data(), texCoords.data(), colors.data(),
                currentIndexCount, indices.data());
            canvas->drawVertices(vertices, SkBlendMode::kModulate, paint);
            currentVertexCount = 0;
            currentIndexCount = 0;
        };


        const float worldHalfWidth = batch.sourceRect.width() * 0.5f * batch.ppuScaleFactor;
        const float worldHalfHeight = batch.sourceRect.height() * 0.5f * batch.ppuScaleFactor;

        const SkPoint srcCorners[] = {
            {batch.sourceRect.fLeft, batch.sourceRect.fTop},
            {batch.sourceRect.fRight, batch.sourceRect.fTop},
            {batch.sourceRect.fRight, batch.sourceRect.fBottom},
            {batch.sourceRect.fLeft, batch.sourceRect.fBottom}
        };

        size_t i = 0;
        const size_t count = batch.count;


        const size_t simdCount = count - (count % simd_chunk_size);
        for (; i < simdCount; i += simd_chunk_size)
        {
            if (currentVertexCount + (simd_chunk_size * 4) > maxVerticesPerDraw) [[unlikely]]
            {
                flushDrawCall();
            }

            const RenderableTransform* t = batch.transforms + i;
            for (size_t k = 0; k < simd_chunk_size; ++k)
            {
                pos_x_soa[k] = t[k].position.fX;
                pos_y_soa[k] = t[k].position.fY;
                scale_x_soa[k] = t[k].scaleX;
                scale_y_soa[k] = t[k].scaleY;
                sin_r_soa[k] = t[k].sinR;
                cos_r_soa[k] = t[k].cosR;
            }


            for (size_t k = 0; k < simd_chunk_size; ++k)
            {
                scaled_half_w[k] = worldHalfWidth * scale_x_soa[k];
                scaled_half_h[k] = worldHalfHeight * scale_y_soa[k];
            }

            const SkPoint localCornerFactors[] = {
                {-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}
            };

            for (int j = 0; j < 4; ++j)
            {
                for (size_t k = 0; k < simd_chunk_size; ++k)
                {
                    scaled_local_x[k] = scaled_half_w[k] * localCornerFactors[j].fX;
                    scaled_local_y[k] = scaled_half_h[k] * localCornerFactors[j].fY;
                }

                simd.VectorRotatePoints(
                    scaled_local_x, scaled_local_y,
                    sin_r_soa, cos_r_soa,
                    final_x, final_y,
                    simd_chunk_size
                );

                simd.VectorAdd(
                    final_x, pos_x_soa,
                    final_x,
                    simd_chunk_size
                );
                simd.VectorAdd(
                    final_y, pos_y_soa,
                    final_y,
                    simd_chunk_size
                );

                for (size_t k = 0; k < simd_chunk_size; ++k)
                {
                    positions[currentVertexCount + k * 4 + j] = {final_x[k], final_y[k]};
                }
            }

            for (size_t k = 0; k < simd_chunk_size; ++k)
            {
                const uint16_t baseVertex = static_cast<uint16_t>(currentVertexCount + k * 4);
                texCoords[baseVertex + 0] = srcCorners[0];
                texCoords[baseVertex + 1] = srcCorners[1];
                texCoords[baseVertex + 2] = srcCorners[2];
                texCoords[baseVertex + 3] = srcCorners[3];

                for (int c = 0; c < 4; ++c) colors[baseVertex + c] = skColor;

                const size_t indexStart = currentIndexCount + k * 6;
                indices[indexStart + 0] = baseVertex + 0;
                indices[indexStart + 1] = baseVertex + 1;
                indices[indexStart + 2] = baseVertex + 2;
                indices[indexStart + 3] = baseVertex + 0;
                indices[indexStart + 4] = baseVertex + 2;
                indices[indexStart + 5] = baseVertex + 3;
            }
            currentVertexCount += (simd_chunk_size * 4);
            currentIndexCount += (simd_chunk_size * 6);
        }


        for (; i < count; ++i)
        {
            if (currentVertexCount + 4 > maxVerticesPerDraw) [[unlikely]]
            {
                flushDrawCall();
            }

            const auto& transform = batch.transforms[i];
            const float s = transform.sinR;
            const float c = transform.cosR;


            const float scaledHalfWidth = worldHalfWidth * transform.scaleX;
            const float scaledHalfHeight = worldHalfHeight * transform.scaleY;

            const SkPoint localCorners[4] = {
                {-scaledHalfWidth, -scaledHalfHeight}, {scaledHalfWidth, -scaledHalfHeight},
                {scaledHalfWidth, scaledHalfHeight}, {-scaledHalfWidth, scaledHalfHeight}
            };

            const uint16_t baseVertex = static_cast<uint16_t>(currentVertexCount);
            for (int j = 0; j < 4; ++j)
            {
                const float rX = localCorners[j].fX * c - localCorners[j].fY * s;
                const float rY = localCorners[j].fX * s + localCorners[j].fY * c;
                positions[currentVertexCount + j] = {transform.position.fX + rX, transform.position.fY + rY};
                texCoords[currentVertexCount + j] = srcCorners[j];
                colors[currentVertexCount + j] = skColor;
            }

            indices[currentIndexCount + 0] = baseVertex + 0;
            indices[currentIndexCount + 1] = baseVertex + 1;
            indices[currentIndexCount + 2] = baseVertex + 2;
            indices[currentIndexCount + 3] = baseVertex + 0;
            indices[currentIndexCount + 4] = baseVertex + 2;
            indices[currentIndexCount + 5] = baseVertex + 3;

            currentVertexCount += 4;
            currentIndexCount += 6;
        }

        flushDrawCall();
    }
}

void RenderSystem::RenderSystemImpl::DrawAllTextBatches(SkCanvas* canvas)
{
    if (textBatches.empty())
    {
        return;
    }

    SkPaint paint;
    paint.setAntiAlias(true);

    SkFont font;
    font.setEdging(SkFont::Edging::kAntiAlias);
    font.setHinting(SkFontHinting::kSlight);


    for (const auto& batch : textBatches)
    {
        if (!batch.typeface || batch.count == 0)
        {
            continue;
        }


        font.setTypeface(batch.typeface);
        font.setSize(batch.fontSize);
        paint.setColor4f(batch.color);

        const float lineHeight = font.getSpacing();
        SkFontMetrics metrics;
        font.getMetrics(&metrics);


        for (size_t i = 0; i < batch.count; ++i)
        {
            const auto& transform = batch.transforms[i];
            const std::string& textBlock = batch.texts[i];

            auto lines = splitStringByNewline(textBlock);
            if (lines.empty())
            {
                continue;
            }


            SkMatrix textMatrix;
            textMatrix.setAll(
                transform.cosR * transform.scaleX, -transform.sinR * transform.scaleX, transform.position.fX,
                transform.sinR * transform.scaleY, transform.cosR * transform.scaleY, transform.position.fY,
                0, 0, 1
            );


            float totalBlockHeight = (lines.size() - 1) * lineHeight - metrics.fAscent + metrics.fDescent;
            float initialYOffset = 0;
            const TextAlignment alignment = static_cast<TextAlignment>(batch.alignment);

            switch (alignment)
            {
            case TextAlignment::TopLeft:
            case TextAlignment::TopCenter:
            case TextAlignment::TopRight:
                initialYOffset = -metrics.fAscent;
                break;
            case TextAlignment::MiddleLeft:
            case TextAlignment::MiddleCenter:
            case TextAlignment::MiddleRight:
                initialYOffset = -totalBlockHeight / 2.0f - metrics.fAscent;
                break;
            case TextAlignment::BottomLeft:
            case TextAlignment::BottomCenter:
            case TextAlignment::BottomRight:
                initialYOffset = -totalBlockHeight - metrics.fAscent;
                break;
            }


            canvas->save();
            canvas->concat(textMatrix);

            for (size_t j = 0; j < lines.size(); ++j)
            {
                const std::string& line = lines[j];
                if (line.empty()) continue;

                SkPoint alignmentOffset = calculateTextAlignmentOffset(line, font, alignment);


                canvas->drawString(line.c_str(),
                                   alignmentOffset.fX,
                                   initialYOffset + j * lineHeight,
                                   font,
                                   paint);
            }

            canvas->restore();
        }
    }
}


struct InstanceGroupKey
{
    const SkImage* image;
    SkColor color;

    bool operator==(const InstanceGroupKey& other) const
    {
        return image == other.image && color == other.color;
    }
};


struct InstanceGroupKeyHash
{
    std::size_t operator()(const InstanceGroupKey& key) const
    {
        const auto hash1 = std::hash<const SkImage*>{}(key.image);
        const auto hash2 = std::hash<SkColor>{}(key.color);
        return hash1 ^ (hash2 << 1);
    }
};


void RenderSystem::RenderSystemImpl::DrawAllInstanceBatches(SkCanvas* canvas)
{
    if (instanceBatches.empty())
    {
        return;
    }


    std::unordered_map<InstanceGroupKey, std::vector<const InstanceBatch*>, InstanceGroupKeyHash> groupedBatches;
    for (const auto& batch : instanceBatches)
    {
        groupedBatches[{batch.atlasImage.get(), batch.color.toSkColor()}].push_back(&batch);
    }


    rsxforms.clear();
    texRects.clear();

    SkPaint paint;
    SkSamplingOptions sampling(SkFilterMode::kLinear);

    for (const auto& [key, batches] : groupedBatches)
    {
        auto atlasImage = sk_ref_sp(key.image);
        paint.setColor(key.color);

        for (const auto* batch : batches)
        {
            for (size_t i = 0; i < batch->count; ++i)
            {
                if (rsxforms.size() >= maxPrimitivesPerBatch) [[unlikely]]
                {
                    canvas->drawAtlas(atlasImage.get(), rsxforms, texRects, {},
                                      SkBlendMode::kModulate, sampling,
                                      nullptr, &paint);
                    rsxforms.clear();
                    texRects.clear();
                }

                const auto& transform = batch->transforms[i];
                const auto& srcRect = batch->sourceRects[i];

                const float s = transform.sinR;
                const float c = transform.cosR;

                const float effectiveScale = (transform.scaleX + transform.scaleY) * 0.5f;

                const SkScalar sc = effectiveScale * c;
                const SkScalar ss = effectiveScale * s;

                const float centerX = srcRect.centerX();
                const float centerY = srcRect.centerY();


                const SkScalar tx = transform.position.fX - (sc * centerX - ss * centerY);
                const SkScalar ty = transform.position.fY - (ss * centerX + sc * centerY);

                rsxforms.push_back(SkRSXform::Make(sc, ss, tx, ty));
                texRects.push_back(srcRect);
            }
        }


        if (!rsxforms.empty())
        {
            canvas->drawAtlas(atlasImage.get(), rsxforms, texRects, {},
                              SkBlendMode::kModulate, sampling,
                              nullptr, &paint);
            rsxforms.clear();
            texRects.clear();
        }
    }
}


void RenderSystem::RenderSystemImpl::DrawAllRectBatches(SkCanvas* canvas)
{
    if (rectBatches.empty()) return;

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    const size_t maxVerticesPerDraw = maxPrimitivesPerBatch * 4;

    positions.clear();
    colors.clear();
    indices.clear();

    for (const auto& batch : rectBatches)
    {
        paint.setColor4f(batch.color, nullptr);
        SkColor skColor = paint.getColor();

        const float halfWidth = batch.size.width() / 2.0f;
        const float halfHeight = batch.size.height() / 2.0f;

        for (size_t i = 0; i < batch.count; ++i)
        {
            if (positions.size() + 4 > maxVerticesPerDraw) [[unlikely]]
            {
                canvas->drawVertices(SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, positions.size(),
                                                          positions.data(), nullptr, colors.data(), indices.size(),
                                                          indices.data()),
                                     SkBlendMode::kSrcOver, paint);
                positions.clear();
                colors.clear();
                indices.clear();
            }

            const auto& transform = batch.transforms[i];
            const float s = transform.sinR;
            const float c = transform.cosR;
            const float scaledHalfWidth = halfWidth * transform.scaleX;
            const float scaledHalfHeight = halfHeight * transform.scaleY;

            const SkPoint localCorners[4] = {
                {-scaledHalfWidth, -scaledHalfHeight}, {scaledHalfWidth, -scaledHalfHeight},
                {scaledHalfWidth, scaledHalfHeight}, {-scaledHalfWidth, scaledHalfHeight}
            };
            uint16_t baseVertex = static_cast<uint16_t>(positions.size());
            for (int j = 0; j < 4; ++j)
            {
                float rX = localCorners[j].fX * c - localCorners[j].fY * s;
                float rY = localCorners[j].fX * s + localCorners[j].fY * c;
                positions.emplace_back(transform.position.fX + rX, transform.position.fY + rY);
                colors.push_back(skColor);
            }
            indices.push_back(baseVertex + 0);
            indices.push_back(baseVertex + 1);
            indices.push_back(baseVertex + 2);
            indices.push_back(baseVertex + 0);
            indices.push_back(baseVertex + 2);
            indices.push_back(baseVertex + 3);
        }
    }

    if (!positions.empty())
    {
        canvas->drawVertices(SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, positions.size(),
                                                  positions.data(), nullptr, colors.data(), indices.size(),
                                                  indices.data()),
                             SkBlendMode::kSrcOver, paint);
    }
}


void RenderSystem::RenderSystemImpl::DrawAllCircleBatches(SkCanvas* canvas)
{
    if (circleBatches.empty()) return;
    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true);
    for (const auto& batch : circleBatches)
    {
        paint.setColor4f(batch.color, nullptr);
        for (size_t i = 0; i < batch.count; ++i)
        {
            canvas->drawCircle(batch.centers[i], batch.radius, paint);
        }
    }
}


void RenderSystem::RenderSystemImpl::DrawAllLineBatches(SkCanvas* canvas)
{
    if (lineBatches.empty()) return;
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setAntiAlias(true);
    for (const auto& batch : lineBatches)
    {
        if (batch.pointCount % 2 != 0) continue;
        paint.setColor4f(batch.color, nullptr);
        paint.setStrokeWidth(batch.width);
        canvas->drawPoints(
            SkCanvas::kLines_PointMode,
            SkSpan<const SkPoint>(batch.points, batch.pointCount),
            paint
        );
    }
}


void RenderSystem::RenderSystemImpl::DrawAllShaderBatches(SkCanvas* canvas)
{
    if (shaderBatches.empty())
    {
        return;
    }

    std::unordered_map<const Material*, std::vector<const ShaderBatch*>> groupedBatches;
    for (const auto& batch : shaderBatches)
    {
        groupedBatches[batch.material].push_back(&batch);
    }

    SkPaint paint;

    paint.setBlendMode(SkBlendMode::kSrc);

    for (const auto& [material, batches] : groupedBatches)
    {
        SkRuntimeShaderBuilder builder(material->effect);
        for (const auto& [name, value] : material->uniforms)
        {
            std::visit(UniformSetter{builder, name}, value);
        }

        sk_sp<SkShader> shader = builder.makeShader();
        if (!shader)
        {
            continue;
        }
        paint.setShader(std::move(shader));

        for (const auto* batch : batches)
        {
            const auto& transform = batch->transform;
            const auto& size = batch->size;

            const SkRect localRect = SkRect::MakeXYWH(-size.width() * 0.5f,
                                                      -size.height() * 0.5f,
                                                      size.width(),
                                                      size.height());
            canvas->save();
            canvas->translate(transform.position.fX, transform.position.fY);
            canvas->rotate(SkRadiansToDegrees(transform.rotation));
            canvas->scale(transform.scale.x(), transform.scale.y());

            canvas->drawRect(localRect, paint);

            canvas->restore();
        }
    }
}

void RenderSystem::RenderSystemImpl::DrawAllCursorBatches(SkCanvas* canvas)
{
    if (cursorPrimitives.empty())
    {
        return;
    }


    positions.clear();
    colors.clear();
    indices.clear();

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);

    constexpr float CURSOR_WIDTH = 1.5f;
    const size_t maxVerticesPerDraw = maxPrimitivesPerBatch * 4;

    auto flushCursorDrawCall = [&]()
    {
        if (positions.empty()) return;

        canvas->drawVertices(SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, positions.size(),
                                                  positions.data(), nullptr, colors.data(), indices.size(),
                                                  indices.data()),
                             SkBlendMode::kSrcOver, paint);
        positions.clear();
        colors.clear();
        indices.clear();
    };

    for (const auto& cursor : cursorPrimitives)
    {
        if (positions.size() + 4 > maxVerticesPerDraw) [[unlikely]]
        {
            flushCursorDrawCall();
        }

        paint.setColor4f(cursor.color, nullptr);
        SkColor skColor = paint.getColor();

        const SkPoint localCorners[4] = {
            {0.0f, 0.0f},
            {CURSOR_WIDTH, 0.0f},
            {CURSOR_WIDTH, cursor.height},
            {0.0f, cursor.height}
        };

        uint16_t baseVertex = static_cast<uint16_t>(positions.size());
        for (int j = 0; j < 4; ++j)
        {
            positions.emplace_back(cursor.position.fX + localCorners[j].fX, cursor.position.fY + localCorners[j].fY);
            colors.push_back(skColor);
        }

        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 3);
    }


    flushCursorDrawCall();
}

void RenderSystem::RenderSystemImpl::DrawAllRawDrawBatches(SkCanvas* canvas)
{
    if (rawDrawBatches.empty()) return;
    for (const auto& batch : rawDrawBatches)
    {
        if (batch.drawFunc)
        {
            batch.drawFunc(canvas);
        }
    }
}
