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
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkColorFilter.h"

namespace
{
    
    inline SkPoint ComputeAnchoredCenter(const ECS::TransformComponent& transform, float width, float height)
    {
        
        float offsetX = (0.5f - transform.anchor.x) * width;
        float offsetY = (0.5f - transform.anchor.y) * height;

        
        if (std::abs(transform.rotation) > 0.0001f)
        {
            const float sinR = sinf(transform.rotation);
            const float cosR = cosf(transform.rotation);
            const float tempX = offsetX;
            offsetX = offsetX * cosR - offsetY * sinR;
            offsetY = tempX * sinR + offsetY * cosR;
        }

        
        return SkPoint::Make(transform.position.x + offsetX, transform.position.y + offsetY);
    }

    
    inline SkSize EstimateTextSize(const std::string& text, float fontSize)
    {
        
        const float charWidth = fontSize * 0.55f; 
        const float lineHeight = fontSize * 1.15f; 

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
    PROFILE_SCOPE("SceneRenderer::Extract - From Manager");

    
    RenderableManager::GetInstance().SetExternalAlpha(1.0f);
    const auto& packets = RenderableManager::GetInstance().GetInterpolationData();

    
    if (outQueue.capacity() < packets.size())
    {
        outQueue.reserve(packets.size());
    }
    outQueue = packets;
}

void SceneRenderer::ExtractToRenderableManager(entt::registry& registry)
{
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Total");

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
            ECS::TransformComponent adjustedTransform = transform;

            const float sourceWidth = sprite.sourceRect.Width() > 0.0f
                                          ? sprite.sourceRect.Width()
                                          : static_cast<float>(sprite.image->getImage()->width());
            const float sourceHeight = sprite.sourceRect.Height() > 0.0f
                                           ? sprite.sourceRect.Height()
                                           : static_cast<float>(sprite.image->getImage()->height());
            const float ppuScaleFactor = (pPU > 0) ? 100.0f / static_cast<float>(pPU) : 1.0f;
            const float worldWidth = sourceWidth * ppuScaleFactor;
            const float worldHeight = sourceHeight * ppuScaleFactor;

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

    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Text Processing");
    {
        auto processTextView = [&](auto& view, auto getTextComponent, auto entity)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                return;

            const auto& transform = view.template get<const ECS::TransformComponent>(entity);
            const auto& textData = getTextComponent(view, entity);

            if (!textData.typeface || textData.text.empty()) return;

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
    }

    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Raw Draw UI Processing");
    {
        auto buttonView = registry.view<const ECS::TransformComponent, const ECS::ButtonComponent>();
        for (auto entity : buttonView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive())
                continue;

            const auto& transform = buttonView.get<const ECS::TransformComponent>(entity);
            const auto& button = buttonView.get<const ECS::ButtonComponent>(entity);

            if (!button.isVisible) continue;

            sk_sp<SkImage> bgImage = (button.backgroundImageTexture && button.backgroundImageTexture->getImage())
                                         ? button.backgroundImageTexture->getImage()
                                         : nullptr;

            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = button.zIndex,
                .transform = transform,
                .data = RawButtonRenderData{
                    .rect = button.rect,
                    .currentState = button.currentState,
                    .normalColor = button.normalColor,
                    .hoverColor = button.hoverColor,
                    .pressedColor = button.pressedColor,
                    .disabledColor = button.disabledColor,
                    .backgroundImage = bgImage,
                    .roundness = button.roundness
                }
            });
        }

        auto inputTextView = registry.view<const ECS::TransformComponent, const ECS::InputTextComponent>();
        for (auto entity : inputTextView)
        {
            if (!SceneManager::GetInstance().GetCurrentScene()->FindGameObjectByEntity(entity).IsActive()) continue;

            const auto& transform = inputTextView.get<const ECS::TransformComponent>(entity);
            const auto& inputText = inputTextView.get<const ECS::InputTextComponent>(entity);

            if (!inputText.text.typeface || !inputText.placeholder.typeface || !inputText.isVisible)
            {
                continue;
            }

            sk_sp<SkImage> bgImage = (inputText.backgroundImageTexture && inputText.backgroundImageTexture->getImage())
                                         ? inputText.backgroundImageTexture->getImage()
                                         : nullptr;

            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = inputText.zIndex,
                .transform = transform,
                .data = RawInputTextRenderData{
                    .rect = inputText.rect,
                    .roundness = inputText.roundness,
                    .normalBackgroundColor = inputText.normalBackgroundColor,
                    .focusedBackgroundColor = inputText.focusedBackgroundColor,
                    .readOnlyBackgroundColor = inputText.readOnlyBackgroundColor,
                    .cursorColor = inputText.cursorColor,
                    .text = inputText.text,
                    .placeholder = inputText.placeholder,
                    .isReadOnly = inputText.isReadOnly,
                    .isFocused = inputText.isFocused,
                    .isPasswordField = inputText.isPasswordField,
                    .isCursorVisible = inputText.isCursorVisible,
                    .cursorPosition = inputText.cursorPosition,
                    .backgroundImage = bgImage,
                    .inputBuffer =inputText.inputBuffer
                }
            });
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