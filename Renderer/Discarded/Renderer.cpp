#define NOMINMAX
#include "Renderer.h"
#include "../GraphicsBackend.h"
#include "Sprite.h"
#include <unordered_map>
#include <algorithm>
#include <ranges>
#include <omp.h>
#include "include/core/SkCanvas.h"
#include "include/core/SkVertices.h"
#include "include/core/SkShader.h"
#include "include/core/SkSpan.h"
#include "include/pathops/SkPathOps.h"
#include "include/core/SkRegion.h"

constexpr int MAX_SPRITES_PER_BATCH = 16384;

void SpriteBatchRenderer::Initialize(GraphicsBackend* backend)
{
    m_backend = backend;
    m_incomingCommands.reserve(MAX_SPRITES_PER_BATCH * 4);
    m_renderQueue.reserve(MAX_SPRITES_PER_BATCH * 4);
    m_positions.resize(MAX_SPRITES_PER_BATCH * 4);
    m_texCoords.resize(MAX_SPRITES_PER_BATCH * 4);
    m_colors.resize(MAX_SPRITES_PER_BATCH * 4);
    m_indices.resize(MAX_SPRITES_PER_BATCH * 6);
    for (uint16_t i = 0; i < MAX_SPRITES_PER_BATCH; ++i)
    {
        uint16_t baseVertex = i * 4;
        size_t index = i * 6;
        m_indices[index + 0] = baseVertex + 0;
        m_indices[index + 1] = baseVertex + 1;
        m_indices[index + 2] = baseVertex + 2;
        m_indices[index + 3] = baseVertex + 0;
        m_indices[index + 4] = baseVertex + 2;
        m_indices[index + 5] = baseVertex + 3;
    }
}

void SpriteBatchRenderer::AddCommands(const std::vector<DrawCommand*>& commands)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_incomingCommands.reserve(m_incomingCommands.size() + commands.size());
    for (const auto* command : commands)
    {
        if (command && command->type == DrawType::Sprite)
        {
            const auto* spriteCmd = static_cast<const SpriteDrawCommand*>(command);

            m_incomingCommands.push_back(*spriteCmd);
        }
    }
}


void SpriteBatchRenderer::Prepare()
{
    if (m_incomingCommands.empty())
    {
        return;
    }


    std::ranges::sort(m_incomingCommands,
                      [](const SpriteDrawCommand& a, const SpriteDrawCommand& b)
                      {
                          const auto& posA = a.transform.GetPosition();
                          const auto& posB = b.transform.GetPosition();
                          if (posA.y != posB.y)
                          {
                              return posA.y < posB.y;
                          }
                          return posA.x < posB.x;
                      });


    m_renderQueue = m_incomingCommands;
}


void SpriteBatchRenderer::Render()
{
    if (m_renderQueue.empty() || !m_backend) return;
    auto surface = m_backend->GetSurface();
    if (!surface) return;
    SkCanvas* canvas = surface->getCanvas();

    std::unordered_map<SkImage*, std::vector<const SpriteDrawCommand*>> batches;
    for (const auto& cmd : m_renderQueue)
    {
        sk_sp<SkImage> image = *cmd.sprite;
        batches[image.get()].push_back(&cmd);
    }

    for (auto const& [imagePtr, commandList] : batches)
    {
        SkPaint paint;
        paint.setShader(sk_ref_sp(imagePtr)->makeShader(SkSamplingOptions(SkFilterMode::kLinear)));
        for (size_t i = 0; i < commandList.size(); i += MAX_SPRITES_PER_BATCH)
        {
            const int spritesInThisBatch = std::min((size_t)MAX_SPRITES_PER_BATCH, commandList.size() - i);
            for (int j = 0; j < spritesInThisBatch; ++j)
            {
                const auto& cmd = *commandList[i + j];
                SkPoint worldPos[4];
                CalculateWorldVertices(cmd, worldPos);
                sk_sp<SkImage> spriteImage = *cmd.sprite;
                const float w = spriteImage->width();
                const float h = spriteImage->height();
                const SkPoint texPos[] = {{0, 0}, {w, 0}, {w, h}, {0, h}};
                const size_t vertexIndex = j * 4;
                for (int k = 0; k < 4; ++k)
                {
                    m_positions[vertexIndex + k] = worldPos[k];
                    m_texCoords[vertexIndex + k] = texPos[k];
                    m_colors[vertexIndex + k] = cmd.color;
                }
            }
            auto vertices = SkVertices::MakeCopy(SkVertices::kTriangles_VertexMode, spritesInThisBatch * 4,
                                                 m_positions.data(), m_texCoords.data(), m_colors.data(),
                                                 spritesInThisBatch * 6, m_indices.data());
            canvas->drawVertices(vertices, SkBlendMode::kSrcOver, paint);
        }
    }
}

void SpriteBatchRenderer::Cleanup()
{
    m_incomingCommands.clear();
    m_renderQueue.clear();
}

Guid SpriteBatchRenderer::HitTest(const Vector2Df& worldPos)
{
    SkPoint testPoint = {worldPos.x, worldPos.y};
    for (auto it = m_renderQueue.rbegin(); it != m_renderQueue.rend(); ++it)
    {
        const auto& cmd = *it;
        SkPoint vertices[4];
        CalculateWorldVertices(cmd, vertices);
        SkPath path;
        path.addPoly(SkSpan(vertices), true);
        if (path.contains(testPoint.x(), testPoint.y()))
        {
            return cmd.GameObjectUID;
        }
    }
    return Guid();
}

void SpriteBatchRenderer::CalculateWorldVertices(const SpriteDrawCommand& cmd, SkPoint worldVerts[4]) const
{
    sk_sp<SkImage> spriteImage = *cmd.sprite;
    const SkMatrix transformMatrix = cmd.transform;
    const float w = spriteImage->width();
    const float h = spriteImage->height();
    const SkPoint localVerts[] = {{0, 0}, {w, 0}, {w, h}, {0, h}};
    transformMatrix.mapPoints(SkSpan(worldVerts, 4), SkSpan(localVerts, 4));
}
