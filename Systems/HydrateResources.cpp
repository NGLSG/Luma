#include "HydrateResources.h"

#include "ColliderComponent.h"
#include "SceneManager.h"
#include "TextComponent.h"
#include "TilemapComponent.h"
#include "Transform.h"
#include "UIComponents.h"
#include "../Resources/RuntimeAsset/RuntimeScene.h"
#include "../Data/EngineContext.h"
#include "../Components/Sprite.h"
#include "../Components/ScriptComponent.h"
#include "../Resources/Loaders/TextureLoader.h"
#include "../Resources/Loaders/MaterialLoader.h"
#include "Event/Events.h"
#include "Loaders/CSharpScriptLoader.h"
#include "Loaders/FontLoader.h"
#include "Loaders/PrefabLoader.h"
#include "Loaders/RuleTileLoader.h"
#include "Loaders/TileLoader.h"

namespace Systems
{
    void HydrateResources::OnCreate(RuntimeScene* scene, EngineContext& context)
    {
        m_context = &context;
        auto& registry = scene->GetRegistry();

        
        auto processEntity = [this, &registry](entt::entity entity)
        {
            if (registry.all_of<ECS::SpriteComponent>(entity)) OnSpriteUpdated(registry, entity);
            if (registry.all_of<ECS::ScriptsComponent>(entity)) OnScriptUpdated(registry, entity);
            if (registry.all_of<ECS::TextComponent>(entity)) OnTextUpdated(registry, entity);
            if (registry.all_of<ECS::ButtonComponent>(entity)) OnButtonUpdated(registry, entity);
            if (registry.all_of<ECS::InputTextComponent>(entity)) OnInputTextUpdated(registry, entity);
            if (registry.all_of<ECS::TilemapComponent>(entity)) OnTilemapUpdated(registry, entity);
        };

        m_listeners.push_back(EventBus::GetInstance().Subscribe<ComponentUpdatedEvent>(
            [this, processEntity](const ComponentUpdatedEvent& event)
            {
                processEntity(event.entity);
            }));

        m_listeners.push_back(EventBus::GetInstance().Subscribe<GameObjectCreatedEvent>(
            [this, processEntity](const GameObjectCreatedEvent& event)
            {
                processEntity(event.entity);
            }));

        m_listeners.push_back(EventBus::GetInstance().Subscribe<ComponentAddedEvent>(
            [this, processEntity](const ComponentAddedEvent& event)
            {
                processEntity(event.entity);
            }));

        m_listeners.push_back(EventBus::GetInstance().Subscribe<CSharpScriptRebuiltEvent>(
            [this](const CSharpScriptRebuiltEvent& event)
            {
                auto& registry = SceneManager::GetInstance().GetCurrentScene()->GetRegistry();
                auto scriptView = registry.view<ECS::ScriptsComponent>();
                for (auto entity : scriptView)
                {
                    OnScriptUpdated(registry, entity);
                }
            }));

        
        for (auto entity : registry.storage<entt::entity>())
        {
            processEntity(entity);
        }
    }

    void HydrateResources::OnUpdate(RuntimeScene* scene, float deltaTime, EngineContext& context)
    {
    }

    void HydrateResources::OnDestroy(RuntimeScene* scene)
    {
        for (auto& listener : m_listeners)
        {
            EventBus::GetInstance().Unsubscribe(listener);
        }
        m_listeners.clear();
        m_context = nullptr;
    }

    void HydrateResources::OnSpriteUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;

        auto& sprite = registry.get<ECS::SpriteComponent>(entity);

        TextureLoader textureLoader(*m_context->graphicsBackend);
        MaterialLoader materialLoader;

        if (sprite.textureHandle.Valid() && (!sprite.image || sprite.lastSpriteHandle != sprite.textureHandle))
        {
            sprite.image = textureLoader.LoadAsset(sprite.textureHandle.assetGuid);
            if (sprite.image)
            {
                sprite.sourceRect = {
                    0, 0, (float)sprite.image->getImage()->width(), (float)sprite.image->getImage()->height()
                };
            }
            else
            {
                LogError("Failed to load texture with GUID: {}", sprite.textureHandle.assetGuid.ToString());
            }
            sprite.lastSpriteHandle = sprite.textureHandle;
        }
        else if (!sprite.textureHandle.Valid())
        {
            sprite.image.reset();
        }

        if (sprite.materialHandle.Valid() && (!sprite.material || sprite.lastMaterialHandle != sprite.materialHandle))
        {
            sprite.material = materialLoader.LoadAsset(sprite.materialHandle.assetGuid);
            if (!sprite.material)
            {
                LogError("Failed to load material with GUID: {}", sprite.materialHandle.assetGuid.ToString());
            }
            sprite.lastMaterialHandle = sprite.materialHandle;
        }
        else if (!sprite.materialHandle.Valid())
        {
            sprite.material.reset();
        }
    }

    void HydrateResources::OnButtonUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;
        auto& button = registry.get<ECS::ButtonComponent>(entity);

        if (button.backgroundImage.Valid() && (!button.backgroundImageTexture || button.backgroundImageTexture->
            GetSourceGuid() != button.backgroundImage.assetGuid))
        {
            TextureLoader textureLoader(*m_context->graphicsBackend);
            button.backgroundImageTexture = textureLoader.LoadAsset(button.backgroundImage.assetGuid);
            if (!button.backgroundImageTexture)
            {
                LogError("Failed to load button background image with GUID: {}",
                         button.backgroundImage.assetGuid.ToString());
            }
        }
        else if (!button.backgroundImage.Valid())
        {
            button.backgroundImageTexture.reset();
        }
    }

    void HydrateResources::OnScriptUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& scriptsComp = registry.get<ECS::ScriptsComponent>(entity);

        for (auto& script : scriptsComp.scripts)
        {
            if (script.scriptAsset.Valid() && (!script.metadata || script.lastScriptAsset != script.scriptAsset))
            {
                CSharpScriptLoader loader;
                sk_sp<RuntimeCSharpScript> scriptAsset = loader.LoadAsset(script.scriptAsset.assetGuid);
                if (scriptAsset)
                {
                    script.metadata = &scriptAsset->GetMetadata();
                }
                else
                {
                    LogError("Failed to load script asset with GUID: {}",
                             script.scriptAsset.assetGuid.ToString());
                    script.metadata = nullptr;
                }
                script.lastScriptAsset = script.scriptAsset;
            }
            else if (!script.scriptAsset.Valid())
            {
                script.metadata = nullptr;
            }
        }
    }

    void HydrateResources::OnTextUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& text = registry.get<ECS::TextComponent>(entity);
        if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
        {
            FontLoader loader;
            text.typeface = loader.LoadAsset(text.fontHandle.assetGuid);
            if (!text.typeface)
            {
                LogError("Failed to load font with GUID: {}", text.fontHandle.assetGuid.ToString());
            }
            text.lastFontHandle = text.fontHandle;
        }
        else if (!text.fontHandle.Valid())
        {
            text.typeface.reset();
            text.lastFontHandle = {};
        }
    }

    void HydrateResources::OnInputTextUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& inputText = registry.get<ECS::InputTextComponent>(entity);
        FontLoader fontLoader;

        
        ECS::TextComponent& text = inputText.text;
        if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
        {
            text.typeface = fontLoader.LoadAsset(text.fontHandle.assetGuid);
            if (!text.typeface)
            {
                LogError("Failed to load font for InputText with GUID: {}", text.fontHandle.assetGuid.ToString());
            }
            text.lastFontHandle = text.fontHandle;
        }
        else if (!text.fontHandle.Valid())
        {
            text.typeface.reset();
            text.lastFontHandle = {};
        }

        
        ECS::TextComponent& placeholder = inputText.placeholder;
        if (placeholder.fontHandle.Valid() && (!placeholder.typeface || placeholder.lastFontHandle !=
            placeholder.fontHandle))
        {
            placeholder.typeface = fontLoader.LoadAsset(placeholder.fontHandle.assetGuid);
            if (!placeholder.typeface)
            {
                LogError("Failed to load font for InputText placeholder with GUID: {}",
                         placeholder.fontHandle.assetGuid.ToString());
            }
            placeholder.lastFontHandle = placeholder.fontHandle;
        }
        else if (!placeholder.fontHandle.Valid())
        {
            placeholder.typeface.reset();
            placeholder.lastFontHandle = {};
        }

        
        if (!m_context || !m_context->graphicsBackend) return;
        if (inputText.backgroundImage.Valid() && (!inputText.backgroundImageTexture || inputText.backgroundImageTexture
            ->GetSourceGuid() != inputText.backgroundImage.assetGuid))
        {
            TextureLoader textureLoader(*m_context->graphicsBackend);
            inputText.backgroundImageTexture = textureLoader.LoadAsset(inputText.backgroundImage.assetGuid);
            if (!inputText.backgroundImageTexture)
            {
                LogError("Failed to load InputText background image with GUID: {}",
                         inputText.backgroundImage.assetGuid.ToString());
            }
        }
        else if (!inputText.backgroundImage.Valid())
        {
            inputText.backgroundImageTexture.reset();
        }
    }

    void HydrateResources::OnTilemapUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;
        auto currentScene = SceneManager::GetInstance().GetCurrentScene();
        if (!currentScene) return;

        auto& tilemap = registry.get<ECS::TilemapComponent>(entity);
        auto tilemapGo = currentScene->FindGameObjectByEntity(entity);
        const auto& tilemapTransform = registry.get<ECS::TransformComponent>(entity);

        TileLoader tileLoader;
        RuleTileLoader ruleTileLoader;

        std::unordered_set<Guid> requiredTileGuids;
        std::unordered_set<Guid> requiredRuleTileGuids;

        for (const auto& [coord, handle] : tilemap.normalTiles)
        {
            if (handle.Valid()) requiredTileGuids.insert(handle.assetGuid);
        }
        for (const auto& [coord, handle] : tilemap.ruleTiles)
        {
            if (handle.Valid()) requiredRuleTileGuids.insert(handle.assetGuid);
        }

        for (const auto& guid : requiredRuleTileGuids)
        {
            if (!guid.Valid()) continue;
            sk_sp<RuntimeRuleTile> ruleTileAsset = ruleTileLoader.LoadAsset(guid);
            if (ruleTileAsset)
            {
                const auto& data = ruleTileAsset->GetData();
                if (data.defaultTileHandle.Valid()) requiredTileGuids.insert(data.defaultTileHandle.assetGuid);
                for (const auto& rule : data.rules)
                {
                    if (rule.resultTileHandle.Valid()) requiredTileGuids.insert(rule.resultTileHandle.assetGuid);
                }
            }
        }

        std::unordered_map<Guid, sk_sp<RuntimeTile>> loadedTiles;
        if (registry.all_of<ECS::TilemapRendererComponent>(entity))
        {
            auto& renderer = registry.get<ECS::TilemapRendererComponent>(entity);
            renderer.hydratedSpriteTiles.clear();
            TextureLoader textureLoader(*m_context->graphicsBackend);

            for (const auto& guid : requiredTileGuids)
            {
                if (!guid.Valid() || loadedTiles.contains(guid)) continue;

                sk_sp<RuntimeTile> tileAsset = tileLoader.LoadAsset(guid);
                loadedTiles[guid] = tileAsset;

                if (tileAsset && std::holds_alternative<SpriteTileData>(tileAsset->GetData()))
                {
                    const auto& spriteData = std::get<SpriteTileData>(tileAsset->GetData());
                    ECS::TilemapRendererComponent::HydratedSpriteTile renderData;
                    renderData.image = textureLoader.LoadAsset(spriteData.textureHandle.assetGuid);

                    if (renderData.image)
                    {
                        if (spriteData.sourceRect.Width() <= 0 || spriteData.sourceRect.Height() <= 0)
                        {
                            renderData.sourceRect = SkRect::MakeWH(renderData.image->getImage()->width(),
                                                                   renderData.image->getImage()->height());
                        }
                        else
                        {
                            renderData.sourceRect = SkRect::MakeXYWH(spriteData.sourceRect.x, spriteData.sourceRect.y,
                                                                     spriteData.sourceRect.Width(),
                                                                     spriteData.sourceRect.Height());
                        }
                    }

                    renderData.color = spriteData.color;
                    renderData.filterQuality = spriteData.filterQuality;
                    renderData.wrapMode = spriteData.wrapMode;
                    renderer.hydratedSpriteTiles[guid] = renderData;
                }
            }
        }

        tilemap.runtimeTileCache.clear();
        std::unordered_set<ECS::Vector2i, ECS::Vector2iHash> requiredPrefabCoords;

        for (const auto& [coord, handle] : tilemap.normalTiles)
        {
            if (loadedTiles.count(handle.assetGuid))
            {
                const auto& tileAsset = loadedTiles.at(handle.assetGuid);
                if (tileAsset)
                {
                    tilemap.runtimeTileCache[coord] = {handle, tileAsset->GetData()};
                    if (std::holds_alternative<PrefabTileData>(tileAsset->GetData()))
                    {
                        requiredPrefabCoords.insert(coord);
                    }
                }
            }
        }

        for (const auto& [coord, handle] : tilemap.ruleTiles)
        {
            sk_sp<RuntimeRuleTile> ruleTileAsset = ruleTileLoader.LoadAsset(handle.assetGuid);
            if (!ruleTileAsset) continue;

            const auto& ruleTileData = ruleTileAsset->GetData();
            AssetHandle selectedTileHandle = ruleTileData.defaultTileHandle;

            const ECS::Vector2i neighborsOffset[8] = {
                {-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}
            };
            for (const auto& rule : ruleTileData.rules)
            {
                bool ruleMatched = true;
                for (int i = 0; i < 8; ++i)
                {
                    if (rule.neighbors[i] == NeighborRule::DontCare) continue;
                    ECS::Vector2i neighborCoord = {coord.x + neighborsOffset[i].x, coord.y + neighborsOffset[i].y};
                    bool neighborIsSameType = false;
                    auto it = tilemap.ruleTiles.find(neighborCoord);
                    if (it != tilemap.ruleTiles.end() && it->second.assetGuid == handle.assetGuid)
                    {
                        neighborIsSameType = true;
                    }
                    if (rule.neighbors[i] == NeighborRule::MustBeThis && !neighborIsSameType)
                    {
                        ruleMatched = false;
                        break;
                    }
                    if (rule.neighbors[i] == NeighborRule::MustNotBeThis && neighborIsSameType)
                    {
                        ruleMatched = false;
                        break;
                    }
                }
                if (ruleMatched)
                {
                    selectedTileHandle = rule.resultTileHandle;
                    break;
                }
            }

            if (selectedTileHandle.Valid() && loadedTiles.count(selectedTileHandle.assetGuid))
            {
                const auto& resultTileAsset = loadedTiles.at(selectedTileHandle.assetGuid);
                if (resultTileAsset)
                {
                    tilemap.runtimeTileCache[coord] = {selectedTileHandle, resultTileAsset->GetData()};
                    if (std::holds_alternative<PrefabTileData>(resultTileAsset->GetData()))
                    {
                        requiredPrefabCoords.insert(coord);
                    }
                }
            }
        }

        std::vector<ECS::Vector2i> coordsToDelete;
        for (const auto& [coord, guid] : tilemap.instantiatedPrefabs)
        {
            if (!requiredPrefabCoords.contains(coord))
            {
                auto go = currentScene->FindGameObjectByGuid(guid);
                if (go.IsValid()) { currentScene->DestroyGameObject(go); }
                coordsToDelete.push_back(coord);
            }
        }
        for (const auto& coord : coordsToDelete) { tilemap.instantiatedPrefabs.erase(coord); }

        PrefabLoader prefabLoader;
        for (const auto& coord : requiredPrefabCoords)
        {
            if (!tilemap.instantiatedPrefabs.contains(coord))
            {
                const auto& data = std::get<PrefabTileData>(tilemap.runtimeTileCache.at(coord).data);
                sk_sp<RuntimePrefab> prefab = prefabLoader.LoadAsset(data.prefabHandle.assetGuid);
                if (prefab)
                {
                    RuntimeGameObject newInstance = currentScene->Instantiate(*prefab, &tilemapGo);
                    if (newInstance.IsValid())
                    {
                        if (newInstance.HasComponent<ECS::TransformComponent>())
                        {
                            auto& transform = newInstance.GetComponent<ECS::TransformComponent>();
                            transform.position = {
                                tilemapTransform.position.x + tilemap.cellSize.x * coord.x,
                                tilemapTransform.position.y + tilemap.cellSize.y * coord.y
                            };
                        }
                        tilemap.instantiatedPrefabs[coord] = newInstance.GetGuid();
                    }
                }
            }
        }

        if (registry.all_of<ECS::TilemapColliderComponent>(entity))
        {
            auto& tilemapCollider = registry.get<ECS::TilemapColliderComponent>(entity);
            tilemapCollider.generatedChains.clear();

            auto isSolid = [&](const ECS::Vector2i& coord) -> bool
            {
                auto it = tilemap.runtimeTileCache.find(coord);
                if (it == tilemap.runtimeTileCache.end())
                {
                    return false;
                }

                return std::visit([](const auto& tileData) -> bool
                {
                    using T = std::decay_t<decltype(tileData)>;
                    if constexpr (std::is_same_v<T, SpriteTileData>)
                    {
                        return true;
                    }
                    return false;
                }, it->second.data);
            };

            std::unordered_map<ECS::Vector2f, std::vector<ECS::Vector2f>, ECS::Vector2fHash> edgeGraph;

            const float cellWidth = tilemap.cellSize.x;
            const float cellHeight = tilemap.cellSize.y;

            for (const auto& [coord, cachedTile] : tilemap.runtimeTileCache)
            {
                if (!isSolid(coord)) continue;

                const ECS::Vector2i neighbors[] = {
                    {coord.x, coord.y - 1},
                    {coord.x, coord.y + 1},
                    {coord.x - 1, coord.y},
                    {coord.x + 1, coord.y}
                };

                ECS::Vector2f topLeft = {coord.x * cellWidth, coord.y * cellHeight};
                ECS::Vector2f topRight = {(coord.x + 1) * cellWidth, coord.y * cellHeight};
                ECS::Vector2f bottomLeft = {coord.x * cellWidth, (coord.y + 1) * cellHeight};
                ECS::Vector2f bottomRight = {(coord.x + 1) * cellWidth, (coord.y + 1) * cellHeight};

                if (!isSolid(neighbors[0]))
                {
                    edgeGraph[topLeft].push_back(topRight);
                    edgeGraph[topRight].push_back(topLeft);
                }

                if (!isSolid(neighbors[1]))
                {
                    edgeGraph[bottomLeft].push_back(bottomRight);
                    edgeGraph[bottomRight].push_back(bottomLeft);
                }

                if (!isSolid(neighbors[2]))
                {
                    edgeGraph[topLeft].push_back(bottomLeft);
                    edgeGraph[bottomLeft].push_back(topLeft);
                }

                if (!isSolid(neighbors[3]))
                {
                    edgeGraph[topRight].push_back(bottomRight);
                    edgeGraph[bottomRight].push_back(topRight);
                }
            }

            while (!edgeGraph.empty())
            {
                std::vector<ECS::Vector2f> newChain;
                auto it = edgeGraph.begin();
                ECS::Vector2f startPoint = it->first;
                ECS::Vector2f currentPoint = startPoint;

                while (true)
                {
                    newChain.push_back(currentPoint);
                    auto& neighbors = edgeGraph.at(currentPoint);

                    if (neighbors.empty())
                    {
                        edgeGraph.erase(currentPoint);
                        break;
                    }

                    ECS::Vector2f nextPoint = neighbors.back();
                    neighbors.pop_back();

                    auto& neighborsOfNext = edgeGraph.at(nextPoint);
                    auto backEdgeIt = std::find(neighborsOfNext.begin(), neighborsOfNext.end(), currentPoint);
                    if (backEdgeIt != neighborsOfNext.end())
                    {
                        neighborsOfNext.erase(backEdgeIt);
                    }

                    if (neighbors.empty())
                    {
                        edgeGraph.erase(currentPoint);
                    }

                    currentPoint = nextPoint;
                }

                if (newChain.size() >= 2)
                {
                    tilemapCollider.generatedChains.push_back(newChain);
                }
            }

            tilemapCollider.isDirty = true;
        }
    }
}