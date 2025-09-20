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


    PROFILE_SCOPE("SceneRenderer::Extract - Optimized Sprite Processing");
    {
        auto spriteView = registry.view<const ECS::TransformComponent, const ECS::SpriteComponent>();


        size_t estimatedSpriteCount = spriteView.size_hint();
        if (estimatedSpriteCount > 0)
        {
            spriteView.each([&](const ECS::TransformComponent& transform, const ECS::SpriteComponent& sprite)
            {
                if (!sprite.image) return;


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


                auto& group = m_spriteBatchGroups[groupIndex];
                group.transforms.emplace_back(RenderableTransform(
                    transform.position,
                    transform.scale.x,
                    transform.scale.y,
                    sinf(transform.rotation),
                    cosf(transform.rotation)
                ));
            });
        }
    }


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

                auto& group = m_spriteBatchGroups[groupIndex];
                group.transforms.emplace_back(RenderableTransform(
                    {
                        tilemapTransform.position.x + coord.x * tilemap.cellSize.x,
                        tilemapTransform.position.y + coord.y * tilemap.cellSize.y
                    },
                    tilemapTransform.scale.x,
                    tilemapTransform.scale.y,
                    sinR,
                    cosR
                ));
            };

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
                group.transforms.emplace_back(RenderableTransform(
                    transform.position,
                    transform.scale.x,
                    transform.scale.y,
                    sinf(transform.rotation),
                    cosf(transform.rotation)
                ));
                group.texts.emplace_back(textData.text);
            }
        };


        auto textView = registry.view<const ECS::TransformComponent, const ECS::TextComponent>();
        processTextView(textView, [](auto& view, auto entity) -> const ECS::TextComponent&
        {
            return view.template get<const ECS::TextComponent>(entity);
        });


        auto inputTextView = registry.view<const ECS::TransformComponent, const ECS::InputTextComponent>();
        processTextView(inputTextView, [](auto& view, auto entity)
        {
            const auto& inputText = view.template get<const ECS::InputTextComponent>(entity);
            return (inputText.isFocused || !inputText.text.text.empty()) ? inputText.text : inputText.placeholder;
        });
    }


    PROFILE_SCOPE("SceneRenderer::Extract - Optimized Packing");
    {
        outQueue.reserve(m_spriteBatchGroups.size() + m_textBatchGroups.size());


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

            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = sprite.zIndex,
                .transform = transform, 
                .data = SpriteRenderData{
                    .image = sprite.image->getImage().get(),
                    .material = sprite.material.get(),
                    .sourceRect = sprite.sourceRect,
                    .color = sprite.color,
                    .filterQuality = static_cast<int>(sprite.image->getImportSettings().filterQuality),
                    .wrapMode = static_cast<int>(sprite.image->getImportSettings().wrapMode),
                    .ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f
                }
            });
        }
    }

    
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
            std::sort(coords.begin(), coords.end(), [](const ECS::Vector2i& a, const ECS::Vector2i& b)
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
                                .ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f
                            }
                        });
                    }
                }
            }
        }
    }

    
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Text Processing");
    {
        
        auto processTextView = [&](auto& view, auto getTextComponent, auto entity)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                return;

            const auto& transform = view.template get<const ECS::TransformComponent>(entity);
            const auto& textData = getTextComponent(view, entity);

            if (!textData.typeface || textData.text.empty()) return;

            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = textData.zIndex,
                .transform = transform,
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
        
        std::stable_sort(renderables.begin(), renderables.end(), [](const Renderable& a, const Renderable& b)
        {
            return static_cast<uint32_t>(a.entityId) < static_cast<uint32_t>(b.entityId);
        });

        RenderableManager::GetInstance().SubmitFrame(std::move(renderables));
    }
}
