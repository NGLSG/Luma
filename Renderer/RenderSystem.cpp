#include "RenderSystem.h"
#include "GraphicsBackend.h"
#include <stdexcept>
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
#include "RuntimeAsset/RuntimeWGSLMaterial.h"
#include "LightingRenderer.h"

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
    std::vector<WGPUSpriteBatch> wgpuSpriteBatches;

    enum class BatchType { Sprite, Text, Instance, Rect, Circle, Line, Shader, RawDraw, WGPUSprite };

    struct QueuedBatch
    {
        BatchType type;
        size_t index;
        RenderSpace renderSpace = RenderSpace::World;
        std::string cameraId;
    };

    std::vector<QueuedBatch> orderedBatches;

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
        wgpuSpriteBatches.reserve(32);
        textBatches.reserve(32);
        instanceBatches.reserve(32);
        rectBatches.reserve(32);
        circleBatches.reserve(32);
        lineBatches.reserve(32);
        shaderBatches.reserve(32);
        rawDrawBatches.reserve(16);
        cursorPrimitives.reserve(16);
        orderedBatches.reserve(128);
    }


    void ClearBatches()
    {
        spriteBatches.clear();
        wgpuSpriteBatches.clear();
        textBatches.clear();
        instanceBatches.clear();
        rectBatches.clear();
        circleBatches.clear();
        lineBatches.clear();
        shaderBatches.clear();
        cursorPrimitives.clear();
        rawDrawBatches.clear();
        orderedBatches.clear();
    }


    void DrawSpriteBatch(const SpriteBatch& batch, SkCanvas* canvas);
    void DrawTextBatch(const TextBatch& batch, SkCanvas* canvas);
    void DrawInstanceBatch(const InstanceBatch& batch, SkCanvas* canvas);
    void DrawRectBatch(const RectBatch& batch, SkCanvas* canvas);
    void DrawCircleBatch(const CircleBatch& batch, SkCanvas* canvas);
    void DrawLineBatch(const LineBatch& batch, SkCanvas* canvas);
    void DrawShaderBatch(const ShaderBatch& batch, SkCanvas* canvas);
    void DrawAllCursorBatches(SkCanvas* canvas);
    void DrawRawDrawBatch(const RawDrawBatch& batch, SkCanvas* canvas);
    void DrawWGPUSpriteBatch(const WGPUSpriteBatch& batch, std::shared_ptr<Nut::NutContext> nutContext);

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

void RenderSystem::Submit(const SpriteBatch& batch)
{
    if (batch.count <= 0) return;
    pImpl->spriteBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Sprite, pImpl->spriteBatches.size() - 1, batch.renderSpace, batch.cameraId});
}

void RenderSystem::Submit(const TextBatch& batch)
{
    if (batch.count <= 0) return;
    pImpl->textBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Text, pImpl->textBatches.size() - 1, batch.renderSpace, batch.cameraId});
}

void RenderSystem::Submit(const InstanceBatch& batch)
{
    if (batch.count <= 0) return;
    pImpl->instanceBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Instance, pImpl->instanceBatches.size() - 1, batch.renderSpace, batch.cameraId});
}

void RenderSystem::Submit(const RectBatch& batch)
{
    if (batch.count <= 0) return;
    pImpl->rectBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Rect, pImpl->rectBatches.size() - 1, batch.renderSpace, batch.cameraId});
}

void RenderSystem::Submit(const CircleBatch& batch)
{
    if (batch.count <= 0) return;
    pImpl->circleBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Circle, pImpl->circleBatches.size() - 1, batch.renderSpace, batch.cameraId});
}

void RenderSystem::Submit(const LineBatch& batch)
{
    if (batch.pointCount < 2) return;
    pImpl->lineBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Line, pImpl->lineBatches.size() - 1, batch.renderSpace, batch.cameraId});
}

void RenderSystem::Submit(const ShaderBatch& batch)
{
    if (batch.material && batch.material->effect)
    {
        pImpl->shaderBatches.push_back(batch);
        pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::Shader, pImpl->shaderBatches.size() - 1, batch.renderSpace, batch.cameraId});
    }
}

void RenderSystem::Submit(const RawDrawBatch& batch)
{
    if (batch.drawFunc)
    {
        pImpl->rawDrawBatches.push_back(batch);
        pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::RawDraw, pImpl->rawDrawBatches.size() - 1, batch.renderSpace, batch.cameraId});
    }
}

void RenderSystem::Submit(const WGPUSpriteBatch& batch)
{
    if (batch.count <= 0) return;
    pImpl->wgpuSpriteBatches.push_back(batch);
    pImpl->orderedBatches.push_back({RenderSystemImpl::BatchType::WGPUSprite, pImpl->wgpuSpriteBatches.size() - 1, batch.renderSpace, batch.cameraId});
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

    // 当前相机状态跟踪
    std::string currentCameraId = DEFAULT_CAMERA_ID;

    auto SetupCanvasState = [&](SkCanvas* cvs)
    {
        if (hasClip)
        {
            const SkRect& viewport = *pImpl->clipRect;
            cvs->save();
            cvs->clipRect(viewport);
            cvs->translate(viewport.fLeft, viewport.fTop);
        }
        cvs->save();
    };

    // 应用指定相机变换
    auto ApplyCamera = [&](SkCanvas* cvs, const std::string& cameraId)
    {
        Camera* camera = CameraManager::GetInstance().GetCamera(cameraId);
        if (camera)
        {
            camera->ApplyTo(cvs);
        }
        else
        {
            // 如果相机不存在，使用激活的相机
            CameraManager::GetInstance().GetActiveCamera().ApplyTo(cvs);
        }
    };

    // 切换相机
    auto SwitchCamera = [&](RenderSpace renderSpace, const std::string& cameraId)
    {
        std::string targetCameraId;
        if (renderSpace == RenderSpace::World)
        {
            targetCameraId = CameraManager::GetInstance().GetActiveCameraId();
        }
        else
        {
            targetCameraId = cameraId.empty() ? UI_CAMERA_ID : cameraId;
        }

        if (currentCameraId == targetCameraId) return;
        
        // 恢复之前的相机变换
        canvas->restore();
        canvas->save();
        
        // 应用新的相机变换
        ApplyCamera(canvas, targetCameraId);
        
        currentCameraId = targetCameraId;
    };

    canvas->clear(CameraManager::GetInstance().GetActiveCamera().GetProperties().clearColor);

    SetupCanvasState(canvas);
    ApplyCamera(canvas, CameraManager::GetInstance().GetActiveCameraId());

    for (const auto& entry : pImpl->orderedBatches)
    {
        if (entry.type == RenderSystemImpl::BatchType::WGPUSprite)
        {
            canvas->restore();
            if (hasClip) canvas->restore();

            pImpl->backend.Submit();

            surface.reset();
            canvas = nullptr;

            pImpl->DrawWGPUSpriteBatch(pImpl->wgpuSpriteBatches[entry.index], pImpl->backend.GetNutContext());

            surface = pImpl->backend.GetSurface();
            if (!surface)
            {
                LogError("Flush: 重建 Surface 失败");
                break;
            }
            canvas = surface->getCanvas();

            SetupCanvasState(canvas);
            ApplyCamera(canvas, currentCameraId);
        }
        else
        {
            // 根据批次的渲染空间切换相机
            SwitchCamera(entry.renderSpace, entry.cameraId);

            switch (entry.type)
            {
            case RenderSystemImpl::BatchType::Sprite:
                pImpl->DrawSpriteBatch(pImpl->spriteBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::Text:
                pImpl->DrawTextBatch(pImpl->textBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::Instance:
                pImpl->DrawInstanceBatch(pImpl->instanceBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::Rect:
                pImpl->DrawRectBatch(pImpl->rectBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::Circle:
                pImpl->DrawCircleBatch(pImpl->circleBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::Line:
                pImpl->DrawLineBatch(pImpl->lineBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::Shader:
                pImpl->DrawShaderBatch(pImpl->shaderBatches[entry.index], canvas);
                break;
            case RenderSystemImpl::BatchType::RawDraw:
                pImpl->DrawRawDrawBatch(pImpl->rawDrawBatches[entry.index], canvas);
                break;
            default: break;
            }
        }
    }

    pImpl->DrawAllCursorBatches(canvas);

    if (canvas)
    {
        canvas->restore();
        if (hasClip)
        {
            canvas->restore();
        }
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


void RenderSystem::RenderSystemImpl::DrawSpriteBatch(const SpriteBatch& singleBatch, SkCanvas* canvas)
{
    if (singleBatch.count == 0)
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


    const SpriteBatch& batch = singleBatch;
    const auto* material = batch.material;
    const auto* image = batch.image.get();
    const SkColor skColor = batch.color.toSkColor();

    if (!image)
    {
        return;
    }

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

    if (!shader)
    {
        return;
    }

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

void RenderSystem::RenderSystemImpl::DrawTextBatch(const TextBatch& singleBatch, SkCanvas* canvas)
{
    if (!singleBatch.typeface || singleBatch.count == 0)
    {
        return;
    }

    SkPaint paint;
    paint.setAntiAlias(true);

    SkFont font;
    font.setEdging(SkFont::Edging::kAntiAlias);
    font.setHinting(SkFontHinting::kSlight);


    const TextBatch& batch = singleBatch;


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


void RenderSystem::RenderSystemImpl::DrawInstanceBatch(const InstanceBatch& batch, SkCanvas* canvas)
{
    if (batch.count == 0 || !batch.atlasImage)
    {
        return;
    }

    rsxforms.clear();
    texRects.clear();

    SkPaint paint;
    paint.setColor4f(batch.color);
    SkSamplingOptions sampling(GetSkFilterMode(batch.filterQuality), GetSkMipmapMode(batch.filterQuality));

    auto atlasImage = batch.atlasImage;

    for (size_t i = 0; i < batch.count; ++i)
    {
        if (rsxforms.size() >= maxPrimitivesPerBatch) [[unlikely]]
        {
            canvas->drawAtlas(atlasImage.get(), rsxforms, texRects, {},
                              SkBlendMode::kModulate, sampling,
                              nullptr, &paint);
            rsxforms.clear();
            texRects.clear();
        }

        const auto& transform = batch.transforms[i];
        const auto& srcRect = batch.sourceRects[i];

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

    if (!rsxforms.empty())
    {
        canvas->drawAtlas(atlasImage.get(), rsxforms, texRects, {},
                          SkBlendMode::kModulate, sampling,
                          nullptr, &paint);
        rsxforms.clear();
        texRects.clear();
    }
}


void RenderSystem::RenderSystemImpl::DrawRectBatch(const RectBatch& singleBatch, SkCanvas* canvas)
{
    if (singleBatch.count == 0) return;

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    const size_t maxVerticesPerDraw = maxPrimitivesPerBatch * 4;

    positions.clear();
    colors.clear();
    indices.clear();

    const RectBatch& batch = singleBatch;
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

    if (!positions.empty())
    {
        canvas->drawVertices(SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, positions.size(),
                                                  positions.data(), nullptr, colors.data(), indices.size(),
                                                  indices.data()),
                             SkBlendMode::kSrcOver, paint);
    }
}


void RenderSystem::RenderSystemImpl::DrawCircleBatch(const CircleBatch& singleBatch, SkCanvas* canvas)
{
    if (singleBatch.count == 0) return;
    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true);
    paint.setColor4f(singleBatch.color, nullptr);
    for (size_t i = 0; i < singleBatch.count; ++i)
    {
        canvas->drawCircle(singleBatch.centers[i], singleBatch.radius, paint);
    }
}


void RenderSystem::RenderSystemImpl::DrawLineBatch(const LineBatch& singleBatch, SkCanvas* canvas)
{
    if (singleBatch.pointCount < 2) return;
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setAntiAlias(true);
    if (singleBatch.pointCount % 2 != 0) return;
    paint.setColor4f(singleBatch.color, nullptr);
    paint.setStrokeWidth(singleBatch.width);
    canvas->drawPoints(
        SkCanvas::kLines_PointMode,
        SkSpan<const SkPoint>(singleBatch.points, singleBatch.pointCount),
        paint
    );
}


void RenderSystem::RenderSystemImpl::DrawShaderBatch(const ShaderBatch& batch, SkCanvas* canvas)
{
    if (!batch.material || !batch.material->effect)
    {
        return;
    }

    SkPaint paint;

    paint.setBlendMode(SkBlendMode::kSrc);
    SkRuntimeShaderBuilder builder(batch.material->effect);
    for (const auto& [name, value] : batch.material->uniforms)
    {
        std::visit(UniformSetter{builder, name}, value);
    }

    sk_sp<SkShader> shader = builder.makeShader();
    if (!shader)
    {
        return;
    }
    paint.setShader(std::move(shader));

    const auto& transform = batch.transform;
    const auto& size = batch.size;

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

void RenderSystem::RenderSystemImpl::DrawRawDrawBatch(const RawDrawBatch& batch, SkCanvas* canvas)
{
    if (!batch.drawFunc) return;
    batch.drawFunc(canvas);
}
void RenderSystem::RenderSystemImpl::DrawWGPUSpriteBatch(const WGPUSpriteBatch& batch,
                                                         std::shared_ptr<Nut::NutContext> nutContext)
{
    if (batch.count == 0) return;

    // 确定是否需要光照
    // 只有当 LightingRenderer 有活跃的 LightingSystem 且有光源时才使用光照
    bool needsLighting = false;
    auto& lightingRenderer = backend.GetLightingRenderer();
    if (lightingRenderer.IsInitialized() && lightingRenderer.GetLightCount() > 0 && batch.lightLayer != 0)
    {
        needsLighting = true;
    }
    
    // 选择材质：
    // 1. 如果提供了自定义材质，使用自定义材质
    // 2. 如果需要光照且没有自定义材质，使用 lit 材质
    // 3. 否则使用默认材质
    RuntimeWGSLMaterial* materialToUse = batch.material;
    if (!materialToUse)
    {
        if (needsLighting)
        {
            materialToUse = backend.CreateOrGetLitMaterial();
        }
        if (!materialToUse)
        {
            materialToUse = backend.CreateOrGetDefaultMaterial();
        }
    }
    
    if (!materialToUse)
    {
        LogError("RenderSystem::RenderSystemImpl::DrawWGPUSpriteBatch: No valid material to use for WGPU sprite batch.");
        return;
    }

    auto swapChainTexture = nutContext->GetCurrentTexture();
    if (!swapChainTexture) return;

    static Nut::Buffer localQuadVBO(std::nullopt);
    static Nut::Buffer localQuadIBO(std::nullopt);
    static std::once_flag geoInit;

    std::call_once(geoInit, [nutContext]()
    {



        std::vector<Vertex> vertices = {

            {-0.5f, -0.5f, 0.0f, 0.0f},
            {-0.5f, 0.5f, 0.0f, 1.0f},
            {0.5f, 0.5f, 1.0f, 1.0f},
            {0.5f, -0.5f, 1.0f, 0.0f}
        };
        std::vector<uint16_t> indices = {0, 1, 2, 0, 2, 3};

        localQuadVBO = Nut::BufferBuilder()
                       .SetUsage(Nut::BufferUsage::Vertex | Nut::BufferUsage::CopyDst)
                       .SetData(vertices)
                       .Build(nutContext);

        localQuadIBO = Nut::BufferBuilder()
                       .SetUsage(Nut::BufferUsage::Index | Nut::BufferUsage::CopyDst)
                       .SetData(indices)
                       .Build(nutContext);
    });

    float texWidth = 1.0f;
    float texHeight = 1.0f;
    if (batch.image)
    {
        texWidth = static_cast<float>(batch.image->GetWidth());
        texHeight = static_cast<float>(batch.image->GetHeight());
    }

    std::vector<InstanceData> instanceDatas(batch.count);
    for (size_t i = 0; i < batch.count; ++i)
    {
        const auto& t = batch.transforms[i];
        const auto& c = batch.color;

        float srcX = batch.sourceRect.fLeft;
        float srcY = batch.sourceRect.fTop;
        float srcW = batch.sourceRect.width();
        float srcH = batch.sourceRect.height();

        if (srcW <= 0.0001f) srcW = texWidth;
        if (srcH <= 0.0001f) srcH = texHeight;

        float uvX = srcX / texWidth;
        float uvY = srcY / texHeight;
        float uvW = srcW / texWidth;
        float uvH = srcH / texHeight;

        float worldW = srcW * batch.ppuScaleFactor;
        float worldH = srcH * batch.ppuScaleFactor;

        instanceDatas[i].position = {t.position.fX, t.position.fY, 0.0f, 1.0f};
        instanceDatas[i].scaleX = t.scaleX;
        instanceDatas[i].scaleY = t.scaleY;
        instanceDatas[i].sinR = t.sinR;
        instanceDatas[i].cosR = t.cosR;
        instanceDatas[i].color = {c.r, c.g, c.b, c.a};
        instanceDatas[i].uvRect = {uvX, uvY, uvW, uvH};
        instanceDatas[i].size = {worldW, worldH};
        instanceDatas[i].lightLayer = batch.lightLayer;
        instanceDatas[i].padding = 0;
        
        // 自发光数据 (Feature: 2d-lighting-enhancement)
        instanceDatas[i].emissionColor = {batch.emissionColor.r, batch.emissionColor.g, batch.emissionColor.b, batch.emissionColor.a};
        instanceDatas[i].emissionIntensity = batch.emissionIntensity;
        instanceDatas[i].emissionPadding1 = 0.0f;
        instanceDatas[i].emissionPadding2 = 0.0f;
        instanceDatas[i].emissionPadding3 = 0.0f;
    }

    EngineData engineData{};
    CameraManager::GetInstance().GetActiveCamera().FillEngineData(engineData);





    engineData.CameraScaleY *= -1.0f;

    if (engineData.ViewportSize.x <= 1 || engineData.ViewportSize.y <= 1)
    {
        engineData.ViewportSize = {
            static_cast<float>(nutContext->GetCurrentSwapChainSize().width),
            static_cast<float>(nutContext->GetCurrentSwapChainSize().height)
        };
    }

    uint32_t sampleCount = backend.GetSampleCount();
    auto msaaTexture = backend.GetMSAATexture();
    bool useMSAA = false;



    if (sampleCount > 1 && msaaTexture && swapChainTexture)
    {
        if (msaaTexture->GetWidth() == swapChainTexture->GetWidth() &&
            msaaTexture->GetHeight() == swapChainTexture->GetHeight())
        {
            useMSAA = true;
        }
    }

    uint32_t targetSampleCount = useMSAA ? sampleCount : 1;
    auto pipeline = materialToUse->GetPipeline(targetSampleCount);

    if (!pipeline)
    {
        LogError("RenderSystem: Failed to get pipeline for sample count {}", targetSampleCount);
        return;
    }

    Nut::Sampler sampler;
    Nut::FilterMode filter = (batch.filterQuality == 0) ? Nut::FilterMode::Nearest : Nut::FilterMode::Linear;
    sampler.SetMagFilter(filter).SetMinFilter(filter).Build(nutContext);

    pipeline->SetReservedBuffers(engineData, instanceDatas, nutContext);
    if (!pipeline->SwapTexture(batch.image, &sampler, nutContext))
    {
        LogError("RenderSystem: Failed to swap texture in material.");
        return;
    }

    // 绑定光照数据到 Group 1
    // 如果需要光照，或者材质使用了 Lighting 模块，都需要绑定光照数据
    // 这样即使没有活跃光源，使用 Lighting 模块的自定义材质也能正常工作
    bool materialNeedsLighting = materialToUse && materialToUse->UsesLightingModule();
    if (needsLighting || materialNeedsLighting)
    {
        auto& lightingRenderer = backend.GetLightingRenderer();
        if (lightingRenderer.IsInitialized())
        {
            // 绑定光照数据到 Group 1，阴影数据到 Group 2，间接光照到 Group 3
            lightingRenderer.BindAllLightingDataWithIndirect(pipeline, 1, 2, 3);
        }
    }

    Nut::ColorAttachmentBuilder attachmentBuilder;

    if (useMSAA)
    {
        attachmentBuilder.SetTexture(msaaTexture)
                         .SetLoadOnOpen(Nut::LoadOnOpen::Load)
                         .SetStoreOnOpen(Nut::StoreOnOpen::Store);
    }
    else
    {
        attachmentBuilder.SetTexture(swapChainTexture)
                         .SetLoadOnOpen(Nut::LoadOnOpen::Load)
                         .SetStoreOnOpen(Nut::StoreOnOpen::Store);
    }

    auto renderPass = nutContext->BeginRenderFrame()
                                .AddColorAttachment(attachmentBuilder.Build())
                                .Build();

    renderPass.SetPipeline(*pipeline);
    materialToUse->Bind(renderPass);

    renderPass.SetVertexBuffer(0, localQuadVBO);
    renderPass.SetIndexBuffer(localQuadIBO, wgpu::IndexFormat::Uint16);

    renderPass.DrawIndexed(6, static_cast<uint32_t>(batch.count), 0, 0, 0);

    nutContext->Submit({nutContext->EndRenderFrame(renderPass)});
}