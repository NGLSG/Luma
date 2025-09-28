#include "SceneRenderer.h"
#include "../Renderer/RenderComponent.h"
#include "../Components/Transform.h"
#include "../Components/Sprite.h"
#include "TextComponent.h"
#include "UIComponents.h"
#include <algorithm>
#include <cmath>

#include "Profiler.h"
#include "RenderableManager.h"
#include "SceneManager.h"
#include "TilemapComponent.h"

namespace
{
    /**
     * @brief 计算基于锚点的中心位置，考虑旋转和缩放。
     *
     * @param transform 变换组件，包含锚点信息。
     * @param width 对象的世界宽度。
     * @param height 对象的世界高度。
     * @return 基于锚点计算的最终渲染位置。
     */
    inline SkPoint ComputeAnchoredCenter(const ECS::TransformComponent& transform, float width, float height)
    {
        // 计算锚点偏移量（基于缩放后的尺寸）
        float offsetX = (0.5f - transform.anchor.x) * width;
        float offsetY = (0.5f - transform.anchor.y) * height;

        // 如果存在旋转，应用旋转变换到偏移量
        if (std::abs(transform.rotation) > 0.0001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);
            const float tempX = offsetX;
            offsetX = offsetX * cosR - offsetY * sinR;
            offsetY = tempX * sinR + offsetY * cosR;
        }

        // 返回最终位置：原始位置 + 锚点偏移
        return SkPoint::Make(transform.position.x + offsetX, transform.position.y + offsetY);
    }

    /**
     * @brief 估算文本的渲染尺寸（针对TextMeshPro风格的文本）。
     *
     * @param text 文本内容。
     * @param fontSize 字体大小。
     * @return 估算的文本尺寸（宽度，高度）。
     */
    inline SkSize EstimateTextSize(const std::string& text, float fontSize)
    {
        // 更精确的文本尺寸估算，适合TextMeshPro风格
        const float charWidth = fontSize * 0.55f; // TextMeshPro字符宽度约为字体大小的0.55倍
        const float lineHeight = fontSize * 1.15f; // TextMeshPro行高约为字体大小的1.15倍

        float maxWidth = 0.0f;
        float currentLineWidth = 0.0f;
        size_t lineCount = 1;

        for (char c : text)
        {
            if (c == '\n')
            {
                maxWidth = std::max(maxWidth, currentLineWidth);
                currentLineWidth = 0.0f;
                lineCount++;
            }
            else
            {
                currentLineWidth += charWidth;
            }
        }

        maxWidth = std::max(maxWidth, currentLineWidth);
        float totalHeight = lineCount * lineHeight;

        return SkSize::Make(maxWidth, totalHeight);
    }
}

void SceneRenderer::Extract(entt::registry& registry, std::vector<RenderPacket>& outQueue)
{
    PROFILE_SCOPE("SceneRenderer::Extract - Total");

    outQueue.clear();
    m_transformArena->Reverse();

    if (m_textArena)
        m_textArena->Reverse();

    m_spriteGroupIndices.clear();
    m_textGroupIndices.clear();
    m_spriteBatchGroups.clear();
    m_textBatchGroups.clear();

    m_spriteGroupIndices.reserve(1000);
    m_textGroupIndices.reserve(100);
    m_spriteBatchGroups.reserve(1000);
    m_textBatchGroups.reserve(100);

    // 处理精灵组件
    PROFILE_SCOPE("SceneRenderer::Extract - Optimized Sprite Processing");
    {
        auto spriteView = registry.view<const ECS::TransformComponent, const ECS::SpriteComponent>();

        size_t estimatedSpriteCount = spriteView.size_hint();
        if (estimatedSpriteCount > 0)
        {
            spriteView.each([&](const ECS::TransformComponent& transform, const ECS::SpriteComponent& sprite)
            {
                if (!sprite.image) return;

                // 创建批处理键
                FastSpriteBatchKey key(
                    sprite.image->getImage().get(),
                    sprite.material.get(),
                    sprite.color,
                    sprite.image->getImportSettings().filterQuality,
                    sprite.image->getImportSettings().wrapMode
                );

                size_t groupIndex;
                auto it = m_spriteGroupIndices.find(key);
                if (it == m_spriteGroupIndices.end())
                {
                    // 创建新的批处理组
                    groupIndex = m_spriteBatchGroups.size();
                    m_spriteGroupIndices[key] = groupIndex;
                    m_spriteBatchGroups.emplace_back();

                    auto& group = m_spriteBatchGroups.back();
                    group.sourceRect = sprite.sourceRect;
                    group.zIndex = sprite.zIndex;
                    group.filterQuality = static_cast<int>(sprite.image->getImportSettings().filterQuality);
                    group.wrapMode = static_cast<int>(sprite.image->getImportSettings().wrapMode);

                    const int pPU = sprite.image->getImportSettings().pixelPerUnit;
                    group.ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f;

                    group.image = sprite.image->getImage().get();
                    group.material = sprite.material.get();
                    group.color = sprite.color;

                    group.transforms.reserve(32);
                }
                else
                {
                    groupIndex = it->second;
                }

                // 计算源矩形尺寸
                auto& group = m_spriteBatchGroups[groupIndex];
                const float sourceWidth = sprite.sourceRect.Width() > 0.0f
                                              ? sprite.sourceRect.Width()
                                              : ((sprite.image && sprite.image->getImage())
                                                     ? static_cast<float>(sprite.image->getImage()->width())
                                                     : 0.0f);
                const float sourceHeight = sprite.sourceRect.Height() > 0.0f
                                               ? sprite.sourceRect.Height()
                                               : ((sprite.image && sprite.image->getImage())
                                                      ? static_cast<float>(sprite.image->getImage()->height())
                                                      : 0.0f);

                // 转换为世界空间尺寸
                const float worldWidth = sourceWidth * group.ppuScaleFactor;
                const float worldHeight = sourceHeight * group.ppuScaleFactor;

                // 使用锚点计算最终渲染位置
                const SkPoint anchoredPos = ComputeAnchoredCenter(transform, worldWidth, worldHeight);

                // 添加变换到批处理组
                group.transforms.emplace_back(RenderableTransform(
                    anchoredPos,
                    transform.scale.x,
                    transform.scale.y,
                    sinf(transform.rotation),
                    cosf(transform.rotation)
                ));
            });
        }
    }

    // 处理瓦片地图组件
    PROFILE_SCOPE("SceneRenderer::Extract - Optimized Tilemap Processing");
    {
        auto tilemapView = registry.view<const ECS::TransformComponent, const ECS::TilemapComponent, const
                                         ECS::TilemapRendererComponent>();

        for (auto entity : tilemapView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;

            const auto& tilemapTransform = tilemapView.get<const ECS::TransformComponent>(entity);
            const auto& tilemap = tilemapView.get<const ECS::TilemapComponent>(entity);
            const auto& renderer = tilemapView.get<const ECS::TilemapRendererComponent>(entity);

            const float sinR = sinf(tilemapTransform.rotation);
            const float cosR = cosf(tilemapTransform.rotation);

            auto batchSpriteTile = [&](const ECS::TilemapRendererComponent::HydratedSpriteTile& hydratedTile,
                                       const ECS::Vector2i& coord)
            {
                if (!hydratedTile.image) return;

                FastSpriteBatchKey key(
                    hydratedTile.image->getImage().get(),
                    renderer.material.get(),
                    hydratedTile.color,
                    hydratedTile.filterQuality,
                    hydratedTile.wrapMode
                );

                size_t groupIndex;
                auto it = m_spriteGroupIndices.find(key);
                if (it == m_spriteGroupIndices.end())
                {
                    groupIndex = m_spriteBatchGroups.size();
                    m_spriteGroupIndices[key] = groupIndex;
                    m_spriteBatchGroups.emplace_back();

                    auto& group = m_spriteBatchGroups.back();
                    group.sourceRect = hydratedTile.sourceRect;
                    group.zIndex = renderer.zIndex;
                    group.filterQuality = static_cast<int>(hydratedTile.filterQuality);
                    group.wrapMode = static_cast<int>(hydratedTile.wrapMode);

                    const int pPU = hydratedTile.image->getImportSettings().pixelPerUnit;
                    group.ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f;

                    group.image = hydratedTile.image->getImage().get();
                    group.material = renderer.material.get();
                    group.color = hydratedTile.color;
                    group.transforms.reserve(32);
                }
                else
                {
                    groupIndex = it->second;
                }

                // 计算瓦片变换（每个瓦片都有自己的位置）
                auto& group = m_spriteBatchGroups[groupIndex];
                ECS::TransformComponent tileTransform = tilemapTransform;
                tileTransform.position.x += coord.x * tilemap.cellSize.x;
                tileTransform.position.y += coord.y * tilemap.cellSize.y;

                const float sourceWidth = hydratedTile.sourceRect.width() > 0.0f
                                              ? hydratedTile.sourceRect.width()
                                              : ((hydratedTile.image && hydratedTile.image->getImage())
                                                     ? static_cast<float>(hydratedTile.image->getImage()->width())
                                                     : 0.0f);
                const float sourceHeight = hydratedTile.sourceRect.height() > 0.0f
                                               ? hydratedTile.sourceRect.height()
                                               : ((hydratedTile.image && hydratedTile.image->getImage())
                                                      ? static_cast<float>(hydratedTile.image->getImage()->height())
                                                      : 0.0f);
                const float worldWidth = sourceWidth * group.ppuScaleFactor;
                const float worldHeight = sourceHeight * group.ppuScaleFactor;

                // 为瓦片应用锚点计算
                const SkPoint anchoredPos = ComputeAnchoredCenter(tileTransform, worldWidth, worldHeight);

                group.transforms.emplace_back(RenderableTransform(
                    anchoredPos,
                    tileTransform.scale.x,
                    tileTransform.scale.y,
                    sinR,
                    cosR
                ));
            };

            // 处理所有瓦片
            for (const auto& [coord, resolvedTile] : tilemap.runtimeTileCache)
            {
                if (std::holds_alternative<SpriteTileData>(resolvedTile.data))
                {
                    const Guid& tileAssetGuid = resolvedTile.sourceTileAsset.assetGuid;
                    if (tileAssetGuid.Valid() && renderer.hydratedSpriteTiles.contains(tileAssetGuid))
                    {
                        batchSpriteTile(renderer.hydratedSpriteTiles.at(tileAssetGuid), coord);
                    }
                }
            }
        }
    }

    // 处理文本组件 - TextMeshPro风格
    PROFILE_SCOPE("SceneRenderer::Extract - Optimized Text Processing");
    {
        auto processTextView = [&](auto& view, auto getTextComponent)
        {
            for (auto entity : view)
            {
                if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                    continue;

                const auto& transform = view.template get<const ECS::TransformComponent>(entity);
                const auto& textData = getTextComponent(view, entity);

                if (!textData.typeface || textData.text.empty()) continue;

                FastTextBatchKey key(textData.typeface.get(), textData.fontSize, textData.alignment, textData.color);

                size_t groupIndex;
                auto it = m_textGroupIndices.find(key);
                if (it == m_textGroupIndices.end())
                {
                    groupIndex = m_textBatchGroups.size();
                    m_textGroupIndices[key] = groupIndex;
                    m_textBatchGroups.emplace_back();

                    auto& group = m_textBatchGroups.back();
                    group.zIndex = textData.zIndex;
                    group.typeface = textData.typeface.get();
                    group.fontSize = textData.fontSize;
                    group.alignment = textData.alignment;
                    group.color = textData.color;
                    group.transforms.reserve(16);
                    group.texts.reserve(16);
                }
                else
                {
                    groupIndex = it->second;
                }

                auto& group = m_textBatchGroups[groupIndex];

                // 为文本应用锚点计算（TextMeshPro风格）
                const SkSize textSize = EstimateTextSize(textData.text, textData.fontSize);
                const SkPoint anchoredPos = ComputeAnchoredCenter(transform, textSize.width(), textSize.height());

                group.transforms.emplace_back(RenderableTransform(
                    anchoredPos,
                    transform.scale.x,
                    transform.scale.y,
                    sinf(transform.rotation),
                    cosf(transform.rotation)
                ));
                group.texts.emplace_back(textData.text);
            }
        };

        // 处理普通文本组件
        auto textView = registry.view<const ECS::TransformComponent, const ECS::TextComponent>();
        processTextView(textView, [](auto& view, auto entity) -> const ECS::TextComponent&
        {
            return view.template get<const ECS::TextComponent>(entity);
        });

        // 处理输入文本组件
        auto inputTextView = registry.view<const ECS::TransformComponent, const ECS::InputTextComponent>();
        processTextView(inputTextView, [](auto& view, auto entity)
        {
            const auto& inputText = view.template get<const ECS::InputTextComponent>(entity);
            return (inputText.isFocused || !inputText.text.text.empty()) ? inputText.text : inputText.placeholder;
        });
    }

    // 打包渲染数据
    PROFILE_SCOPE("SceneRenderer::Extract - Optimized Packing");
    {
        outQueue.reserve(m_spriteBatchGroups.size() + m_textBatchGroups.size());

        // 打包精灵批处理数据
        for (const auto& group : m_spriteBatchGroups)
        {
            const size_t count = group.transforms.size();
            if (count == 0) continue;

            RenderableTransform* transformBuffer = m_transformArena->Allocate(count);
            std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

            outQueue.emplace_back(RenderPacket{
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

        // 打包文本批处理数据
        for (const auto& group : m_textBatchGroups)
        {
            const size_t count = group.transforms.size();
            if (count == 0) continue;

            RenderableTransform* transformBuffer = m_transformArena->Allocate(count);
            std::memcpy(transformBuffer, group.transforms.data(), count * sizeof(RenderableTransform));

            std::string* textBuffer = m_textArena->Allocate(count);
            for (size_t i = 0; i < count; ++i)
            {
                textBuffer[i] = group.texts[i];
            }

            outQueue.emplace_back(RenderPacket{
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
    }

    // 按Z轴深度排序
    PROFILE_SCOPE("SceneRenderer::Extract - Sorting");
    {
        std::ranges::sort(outQueue, [](const RenderPacket& a, const RenderPacket& b)
        {
            return a.zIndex < b.zIndex;
        });
    }
}

void SceneRenderer::ExtractToRenderableManager(entt::registry& registry)
{
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Total");

    auto& renderableManager = RenderableManager::GetInstance();

    std::vector<Renderable> renderables;

    // 处理精灵组件到RenderableManager
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Sprite Processing");
    {
        auto view = registry.view<const ECS::TransformComponent, const ECS::SpriteComponent>();
        for (auto entity : view)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;

            const auto& transform = view.get<const ECS::TransformComponent>(entity);
            const auto& sprite = view.get<const ECS::SpriteComponent>(entity);

            if (!sprite.image || !sprite.image->getImage()) continue;

            const int pPU = sprite.image->getImportSettings().pixelPerUnit;

            // 为RenderableManager也应用锚点变换
            ECS::TransformComponent adjustedTransform = transform;

            // 计算世界空间尺寸
            const float sourceWidth = sprite.sourceRect.Width() > 0.0f
                ? sprite.sourceRect.Width()
                : static_cast<float>(sprite.image->getImage()->width());
            const float sourceHeight = sprite.sourceRect.Height() > 0.0f
                ? sprite.sourceRect.Height()
                : static_cast<float>(sprite.image->getImage()->height());
            const float ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f;
            const float worldWidth = sourceWidth * ppuScaleFactor;
            const float worldHeight = sourceHeight * ppuScaleFactor;

            // 应用锚点调整
            const SkPoint anchoredPos = ComputeAnchoredCenter(transform, worldWidth, worldHeight);
            adjustedTransform.position = ECS::Vector2f(anchoredPos.x(), anchoredPos.y());

            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = sprite.zIndex,
                .transform = adjustedTransform,
                .data = SpriteRenderData{
                    .image = sprite.image->getImage().get(),
                    .material = sprite.material.get(),
                    .sourceRect = sprite.sourceRect,
                    .color = sprite.color,
                    .filterQuality = static_cast<int>(sprite.image->getImportSettings().filterQuality),
                    .wrapMode = static_cast<int>(sprite.image->getImportSettings().wrapMode),
                    .ppuScaleFactor = ppuScaleFactor
                }
            });
        }
    }

    // 处理瓦片地图组件到RenderableManager
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Tilemap Processing");
    {
        auto view = registry.view<const ECS::TransformComponent, const ECS::TilemapComponent, const
                                  ECS::TilemapRendererComponent>();
        for (auto entity : view)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;

            const auto& tilemapTransform = view.get<const ECS::TransformComponent>(entity);
            const auto& tilemap = view.get<const ECS::TilemapComponent>(entity);
            const auto& renderer = view.get<const ECS::TilemapRendererComponent>(entity);

            std::vector<ECS::Vector2i> coords;
            coords.reserve(tilemap.runtimeTileCache.size());
            for (const auto& kv : tilemap.runtimeTileCache)
            {
                coords.push_back(kv.first);
            }
            std::ranges::sort(coords, [](const ECS::Vector2i& a, const ECS::Vector2i& b)
            {
                if (a.x != b.x) return a.x < b.x;
                return a.y < b.y;
            });

            for (const auto& coord : coords)
            {
                const auto& resolvedTile = tilemap.runtimeTileCache.at(coord);
                if (std::holds_alternative<SpriteTileData>(resolvedTile.data))
                {
                    const Guid& tileAssetGuid = resolvedTile.sourceTileAsset.assetGuid;
                    if (tileAssetGuid.Valid() && renderer.hydratedSpriteTiles.contains(tileAssetGuid))
                    {
                        const auto& hydratedTile = renderer.hydratedSpriteTiles.at(tileAssetGuid);
                        if (!hydratedTile.image) continue;

                        ECS::TransformComponent tileTransform = tilemapTransform;
                        tileTransform.position.x += coord.x * tilemap.cellSize.x;
                        tileTransform.position.y += coord.y * tilemap.cellSize.y;

                        const int pPU = hydratedTile.image->getImportSettings().pixelPerUnit;
                        const float ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f;

                        // 为瓦片也应用锚点计算
                        const float sourceWidth = hydratedTile.sourceRect.width() > 0.0f
                            ? hydratedTile.sourceRect.width()
                            : static_cast<float>(hydratedTile.image->getImage()->width());
                        const float sourceHeight = hydratedTile.sourceRect.height() > 0.0f
                            ? hydratedTile.sourceRect.height()
                            : static_cast<float>(hydratedTile.image->getImage()->height());
                        const float worldWidth = sourceWidth * ppuScaleFactor;
                        const float worldHeight = sourceHeight * ppuScaleFactor;

                        const SkPoint anchoredPos = ComputeAnchoredCenter(tileTransform, worldWidth, worldHeight);
                        tileTransform.position = ECS::Vector2f(anchoredPos.x(), anchoredPos.y());

                        renderables.emplace_back(Renderable{
                            .entityId = entity,
                            .zIndex = renderer.zIndex,
                            .transform = tileTransform,
                            .data = SpriteRenderData{
                                .image = hydratedTile.image->getImage().get(),
                                .material = renderer.material.get(),
                                .sourceRect = hydratedTile.sourceRect,
                                .color = hydratedTile.color,
                                .filterQuality = static_cast<int>(hydratedTile.filterQuality),
                                .wrapMode = static_cast<int>(hydratedTile.wrapMode),
                                .ppuScaleFactor = ppuScaleFactor
                            }
                        });
                    }
                }
            }
        }
    }

    // 处理文本组件到RenderableManager
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Text Processing");
    {
        auto processTextView = [&](auto& view, auto getTextComponent, auto entity)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                return;

            const auto& transform = view.template get<const ECS::TransformComponent>(entity);
            const auto& textData = getTextComponent(view, entity);

            if (!textData.typeface || textData.text.empty()) return;

            // 为文本也应用锚点变换
            ECS::TransformComponent adjustedTransform = transform;
            const SkSize textSize = EstimateTextSize(textData.text, textData.fontSize);
            const SkPoint anchoredPos = ComputeAnchoredCenter(transform, textSize.width(), textSize.height());
            adjustedTransform.position = ECS::Vector2f(anchoredPos.x(), anchoredPos.y());

            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = textData.zIndex,
                .transform = adjustedTransform,
                .data = TextRenderData{
                    .typeface = textData.typeface.get(),
                    .text = textData.text,
                    .fontSize = textData.fontSize,
                    .color = textData.color,
                    .alignment = static_cast<int>(textData.alignment)
                }
            });
        };

        auto textView = registry.view<const ECS::TransformComponent, const ECS::TextComponent>();
        for (auto entity : textView)
        {
            processTextView(textView, [](auto& v, auto e) -> const ECS::TextComponent&
            {
                return v.template get<const ECS::TextComponent>(e);
            }, entity);
        }

        auto inputTextView = registry.view<const ECS::TransformComponent, const ECS::InputTextComponent>();
        for (auto entity : inputTextView)
        {
            processTextView(inputTextView, [](auto& v, auto e)
            {
                const auto& inputText = v.template get<const ECS::InputTextComponent>(e);
                return (inputText.isFocused || !inputText.text.text.empty()) ? inputText.text : inputText.placeholder;
            }, entity);
        }
    }

    if (!renderables.empty())
    {
        std::ranges::stable_sort(renderables, [](const Renderable& a, const Renderable& b)
        {
            return static_cast<uint32_t>(a.entityId) < static_cast<uint32_t>(b.entityId);
        });

        RenderableManager::GetInstance().SubmitFrame(std::move(renderables));
    }
}