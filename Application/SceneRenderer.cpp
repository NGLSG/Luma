#include "SceneRenderer.h"
#include "../Renderer/RenderComponent.h"
#include "../Components/Transform.h"
#include "../Components/Sprite.h"
#include "../Components/LayerComponent.h"
#include "TextComponent.h"
#include "UIComponents.h"
#include "../Components/RelationshipComponent.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include "Profiler.h"
#include "RenderableManager.h"
#include "SceneManager.h"
#include "TilemapComponent.h"
#include "../Resources/RuntimeAsset/RuntimeGameObject.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkColorFilter.h"
namespace
{
    inline SkPoint ComputeAnchoredCenter(const ECS::TransformComponent& transform, float width, float height)
    {
        float offsetX = (0.5f - transform.anchor.x) * width;
        float offsetY = (0.5f - transform.anchor.y) * height;
        offsetX *= transform.scale.x;
        offsetY *= transform.scale.y;
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
    void BuildHierarchyDrawOrder(const sk_sp<RuntimeScene>& scene,
                                 std::unordered_map<entt::entity, uint64_t>& outOrder)
    {
        if (!scene)
        {
            return;
        }
        RuntimeScene* scenePtr = scene.get();
        uint64_t orderCounter = 0;
        std::function<void(RuntimeGameObject&)> traverse = [&](RuntimeGameObject& go)
        {
            if (!go.IsValid()) return;
            outOrder[go.GetEntityHandle()] = orderCounter++;
            if (go.HasComponent<ECS::ChildrenComponent>())
            {
                const auto& children = go.GetComponent<ECS::ChildrenComponent>().children;
                for (auto childEntity : children)
                {
                    RuntimeGameObject child(childEntity, scenePtr);
                    traverse(child);
                }
            }
        };
        auto& roots = scene->GetRootGameObjects();
        for (auto& go : roots)
        {
            if (go.IsValid())
            {
                traverse(go);
            }
        }
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
    sk_sp<RuntimeScene> currentScene = SceneManager::GetInstance().GetCurrentScene();
    std::unordered_map<entt::entity, uint64_t> hierarchyOrder;
    BuildHierarchyDrawOrder(currentScene, hierarchyOrder);
    uint64_t fallbackOrder = hierarchyOrder.size();
    auto getSortKey = [&](entt::entity entity) -> uint64_t
    {
        if (auto it = hierarchyOrder.find(entity); it != hierarchyOrder.end())
        {
            return it->second;
        }
        return fallbackOrder++;
    };
    std::vector<Renderable> renderables;
    PROFILE_SCOPE("SceneRenderer::ExtractToRenderableManager - Sprite Processing");
    {
        auto view = registry.view<const ECS::TransformComponent, const ECS::SpriteComponent>();
        for (auto entity : view)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive())
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
                .sortKey = getSortKey(entity),
                .transform = adjustedTransform,
                .data = SpriteRenderData{
                    .image = sprite.image->getImage().get(),
                    .material = sprite.material.get(),
                    .wgpuTexture = sprite.image->getNutTexture(),
                    .wgpuMaterial = sprite.wgslMaterial.get(),
                    .sourceRect = sprite.sourceRect,
                    .color = sprite.color,
                    .filterQuality = static_cast<int>(sprite.image->getImportSettings().filterQuality),
                    .wrapMode = static_cast<int>(sprite.image->getImportSettings().wrapMode),
                    .ppuScaleFactor = ppuScaleFactor,
                    .isUISprite = sprite.image->getNutTexture() ? false : true,
                    // 优先使用 LayerComponent，否则使用 Sprite 的 lightLayer
                    .lightLayer = registry.any_of<ECS::LayerComponent>(entity) 
                        ? registry.get<ECS::LayerComponent>(entity).GetLayerMask()
                        : sprite.lightLayer.value,
                    // 自发光数据 (Feature: 2d-lighting-enhancement)
                    .emissionColor = sprite.emissionColor,
                    .emissionIntensity = sprite.emissionIntensity
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
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive())
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
                            .sortKey = getSortKey(entity),
                            .transform = tileTransform,
                            .data = SpriteRenderData{
                                .image = hydratedTile.image->getImage().get(),
                                .material = renderer.material.get(),
                                .wgpuTexture = hydratedTile.image->getNutTexture(),
                                .wgpuMaterial = nullptr,
                                .sourceRect = hydratedTile.sourceRect,
                                .color = hydratedTile.color,
                                .filterQuality = static_cast<int>(hydratedTile.filterQuality),
                                .wrapMode = static_cast<int>(hydratedTile.wrapMode),
                                .ppuScaleFactor = ppuScaleFactor,
                                .isUISprite = false,
                                .lightLayer = 0xFFFFFFFF // Tilemaps use default light layer
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
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive())
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
                .sortKey = getSortKey(entity),
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
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive())
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
                .sortKey = getSortKey(entity),
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
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
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
                .sortKey = getSortKey(entity),
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
                    .inputBuffer = inputText.inputBuffer
                }
            });
        }
        auto toggleView = registry.view<const ECS::TransformComponent, const ECS::ToggleButtonComponent>();
        for (auto entity : toggleView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = toggleView.get<const ECS::TransformComponent>(entity);
            const auto& toggle = toggleView.get<const ECS::ToggleButtonComponent>(entity);
            if (!toggle.isVisible) continue;
            sk_sp<SkImage> bgImage = (toggle.backgroundImageTexture && toggle.backgroundImageTexture->getImage())
                                         ? toggle.backgroundImageTexture->getImage()
                                         : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = toggle.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawToggleButtonRenderData{
                    .rect = toggle.rect,
                    .currentState = toggle.currentState,
                    .isToggled = toggle.isToggled,
                    .normalColor = toggle.normalColor,
                    .hoverColor = toggle.hoverColor,
                    .pressedColor = toggle.pressedColor,
                    .toggledColor = toggle.toggledColor,
                    .toggledHoverColor = toggle.toggledHoverColor,
                    .toggledPressedColor = toggle.toggledPressedColor,
                    .disabledColor = toggle.disabledColor,
                    .backgroundImage = bgImage,
                    .roundness = toggle.roundness
                }
            });
        }
        auto radioView = registry.view<const ECS::TransformComponent, const ECS::RadioButtonComponent>();
        for (auto entity : radioView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = radioView.get<const ECS::TransformComponent>(entity);
            const auto& radio = radioView.get<const ECS::RadioButtonComponent>(entity);
            if (!radio.isVisible) continue;
            if (!radio.label.typeface) continue;
            sk_sp<SkImage> bgImage = (radio.backgroundImageTexture && radio.backgroundImageTexture->getImage())
                                         ? radio.backgroundImageTexture->getImage()
                                         : nullptr;
            sk_sp<SkImage> selectionImage = (radio.selectionImageTexture && radio.selectionImageTexture->getImage())
                                                ? radio.selectionImageTexture->getImage()
                                                : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = radio.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawRadioButtonRenderData{
                    .rect = radio.rect,
                    .currentState = radio.currentState,
                    .isSelected = radio.isSelected,
                    .normalColor = radio.normalColor,
                    .hoverColor = radio.hoverColor,
                    .selectedColor = radio.selectedColor,
                    .disabledColor = radio.disabledColor,
                    .indicatorColor = radio.indicatorColor,
                    .label = radio.label,
                    .backgroundImage = bgImage,
                    .selectionImage = selectionImage,
                    .roundness = radio.roundness
                }
            });
        }
        auto checkBoxView = registry.view<const ECS::TransformComponent, const ECS::CheckBoxComponent>();
        for (auto entity : checkBoxView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = checkBoxView.get<const ECS::TransformComponent>(entity);
            const auto& checkBox = checkBoxView.get<const ECS::CheckBoxComponent>(entity);
            if (!checkBox.isVisible) continue;
            if (!checkBox.label.typeface) continue;
            sk_sp<SkImage> bgImage = (checkBox.backgroundImageTexture && checkBox.backgroundImageTexture->getImage())
                                         ? checkBox.backgroundImageTexture->getImage()
                                         : nullptr;
            sk_sp<SkImage> checkmarkImage = (checkBox.checkmarkImageTexture && checkBox.checkmarkImageTexture->
                                                getImage())
                                                ? checkBox.checkmarkImageTexture->getImage()
                                                : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = checkBox.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawCheckBoxRenderData{
                    .rect = checkBox.rect,
                    .currentState = checkBox.currentState,
                    .isChecked = checkBox.isChecked,
                    .isIndeterminate = checkBox.isIndeterminate,
                    .normalColor = checkBox.normalColor,
                    .hoverColor = checkBox.hoverColor,
                    .checkedColor = checkBox.checkedColor,
                    .indeterminateColor = checkBox.indeterminateColor,
                    .disabledColor = checkBox.disabledColor,
                    .checkmarkColor = checkBox.checkmarkColor,
                    .label = checkBox.label,
                    .backgroundImage = bgImage,
                    .checkmarkImage = checkmarkImage,
                    .roundness = checkBox.roundness
                }
            });
        }
        auto sliderView = registry.view<const ECS::TransformComponent, const ECS::SliderComponent>();
        for (auto entity : sliderView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = sliderView.get<const ECS::TransformComponent>(entity);
            const auto& slider = sliderView.get<const ECS::SliderComponent>(entity);
            if (!slider.isVisible) continue;
            sk_sp<SkImage> trackImage = (slider.trackImageTexture && slider.trackImageTexture->getImage())
                                            ? slider.trackImageTexture->getImage()
                                            : nullptr;
            sk_sp<SkImage> fillImage = (slider.fillImageTexture && slider.fillImageTexture->getImage())
                                           ? slider.fillImageTexture->getImage()
                                           : nullptr;
            sk_sp<SkImage> thumbImage = (slider.thumbImageTexture && slider.thumbImageTexture->getImage())
                                            ? slider.thumbImageTexture->getImage()
                                            : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = slider.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawSliderRenderData{
                    .rect = slider.rect,
                    .isVertical = slider.isVertical,
                    .isDragging = slider.isDragging,
                    .isInteractable = slider.isInteractable && slider.Enable,
                    .normalizedValue = slider.normalizedValue,
                    .trackColor = slider.trackColor,
                    .fillColor = slider.fillColor,
                    .thumbColor = slider.thumbColor,
                    .disabledColor = slider.disabledColor,
                    .trackImage = trackImage,
                    .fillImage = fillImage,
                    .thumbImage = thumbImage
                }
            });
        }
        auto comboView = registry.view<const ECS::TransformComponent, const ECS::ComboBoxComponent>();
        for (auto entity : comboView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = comboView.get<const ECS::TransformComponent>(entity);
            const auto& combo = comboView.get<const ECS::ComboBoxComponent>(entity);
            if (!combo.isVisible) continue;
            if (!combo.displayText.typeface) continue;
            sk_sp<SkImage> bgImage = (combo.backgroundImageTexture && combo.backgroundImageTexture->getImage())
                                         ? combo.backgroundImageTexture->getImage()
                                         : nullptr;
            sk_sp<SkImage> iconImage = (combo.dropdownIconTexture && combo.dropdownIconTexture->getImage())
                                           ? combo.dropdownIconTexture->getImage()
                                           : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = combo.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawComboBoxRenderData{
                    .rect = combo.rect,
                    .currentState = combo.currentState,
                    .isDropdownOpen = combo.isDropdownOpen,
                    .selectedIndex = combo.selectedIndex,
                    .hoveredIndex = combo.hoveredIndex,
                    .displayText = combo.displayText,
                    .items = combo.items,
                    .normalColor = combo.normalColor,
                    .hoverColor = combo.hoverColor,
                    .pressedColor = combo.pressedColor,
                    .disabledColor = combo.disabledColor,
                    .dropdownBackgroundColor = combo.dropdownBackgroundColor,
                    .backgroundImage = bgImage,
                    .dropdownIcon = iconImage,
                    .roundness = combo.roundness
                }
            });
        }
        auto expanderView = registry.view<const ECS::TransformComponent, const ECS::ExpanderComponent>();
        for (auto entity : expanderView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = expanderView.get<const ECS::TransformComponent>(entity);
            const auto& expander = expanderView.get<const ECS::ExpanderComponent>(entity);
            if (!expander.isVisible) continue;
            if (!expander.header.typeface) continue;
            sk_sp<SkImage> bgImage = (expander.backgroundImageTexture && expander.backgroundImageTexture->getImage())
                                         ? expander.backgroundImageTexture->getImage()
                                         : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = expander.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawExpanderRenderData{
                    .rect = expander.rect,
                    .isExpanded = expander.isExpanded,
                    .header = expander.header,
                    .headerColor = expander.headerColor,
                    .expandedColor = expander.expandedColor,
                    .collapsedColor = expander.collapsedColor,
                    .disabledColor = expander.disabledColor,
                    .backgroundImage = bgImage,
                    .roundness = expander.roundness
                }
            });
        }
        auto progressView = registry.view<const ECS::TransformComponent, const ECS::ProgressBarComponent>();
        for (auto entity : progressView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = progressView.get<const ECS::TransformComponent>(entity);
            const auto& progress = progressView.get<const ECS::ProgressBarComponent>(entity);
            if (!progress.isVisible) continue;
            sk_sp<SkImage> bgImage = (progress.backgroundImageTexture && progress.backgroundImageTexture->getImage())
                                         ? progress.backgroundImageTexture->getImage()
                                         : nullptr;
            sk_sp<SkImage> fillImage = (progress.fillImageTexture && progress.fillImageTexture->getImage())
                                           ? progress.fillImageTexture->getImage()
                                           : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = progress.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawProgressBarRenderData{
                    .rect = progress.rect,
                    .minValue = progress.minValue,
                    .maxValue = progress.maxValue,
                    .value = progress.value,
                    .showPercentage = progress.showPercentage,
                    .isIndeterminate = progress.isIndeterminate,
                    .indeterminatePhase = progress.indeterminatePhase,
                    .backgroundColor = progress.backgroundColor,
                    .fillColor = progress.fillColor,
                    .borderColor = progress.borderColor,
                    .backgroundImage = bgImage,
                    .fillImage = fillImage
                }
            });
        }
        auto tabView = registry.view<const ECS::TransformComponent, const ECS::TabControlComponent>();
        for (auto entity : tabView)
        {
            if (currentScene && !currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
            const auto& transform = tabView.get<const ECS::TransformComponent>(entity);
            const auto& tabControl = tabView.get<const ECS::TabControlComponent>(entity);
            if (!tabControl.isVisible) continue;
            sk_sp<SkImage> bgImage = (tabControl.backgroundImageTexture && tabControl.backgroundImageTexture->
                                         getImage())
                                         ? tabControl.backgroundImageTexture->getImage()
                                         : nullptr;
            sk_sp<SkImage> tabBgImage = (tabControl.tabBackgroundImageTexture && tabControl.tabBackgroundImageTexture->
                                            getImage())
                                            ? tabControl.tabBackgroundImageTexture->getImage()
                                            : nullptr;
            renderables.emplace_back(Renderable{
                .entityId = entity,
                .zIndex = tabControl.zIndex,
                .sortKey = getSortKey(entity),
                .transform = transform,
                .data = RawTabControlRenderData{
                    .rect = tabControl.rect,
                    .tabs = tabControl.tabs,
                    .activeTabIndex = tabControl.activeTabIndex,
                    .hoveredTabIndex = tabControl.hoveredTabIndex,
                    .tabHeight = tabControl.tabHeight,
                    .tabSpacing = tabControl.tabSpacing,
                    .backgroundColor = tabControl.backgroundColor,
                    .tabColor = tabControl.tabColor,
                    .activeTabColor = tabControl.activeTabColor,
                    .hoverTabColor = tabControl.hoverTabColor,
                    .disabledTabColor = tabControl.disabledTabColor,
                    .backgroundImage = bgImage,
                    .tabBackgroundImage = tabBgImage
                }
            });
        }
        if (currentScene)
        {
            auto listBoxView = registry.view<const ECS::TransformComponent, const ECS::ListBoxComponent>();
            for (auto entity : listBoxView)
            {
                if (!currentScene->FindGameObjectByEntity(entity).IsActive()) continue;
                const auto& transform = listBoxView.get<const ECS::TransformComponent>(entity);
                const auto& listBox = listBoxView.get<const ECS::ListBoxComponent>(entity);
                if (!listBox.isVisible) continue;
                bool useContainer = listBox.itemsContainerGuid.Valid();
                int itemCount = static_cast<int>(listBox.items.size());
                if (useContainer)
                {
                    itemCount = 0;
                    RuntimeGameObject containerGO = currentScene->FindGameObjectByGuid(listBox.itemsContainerGuid);
                    if (containerGO.IsValid())
                    {
                        entt::entity containerEntity = static_cast<entt::entity>(containerGO);
                        if (registry.valid(containerEntity) && registry.any_of<ECS::ChildrenComponent>(containerEntity))
                        {
                            const auto& childrenComp = registry.get<ECS::ChildrenComponent>(containerEntity);
                            for (auto childEntity : childrenComp.children)
                            {
                                if (!registry.valid(childEntity)) continue;
                                if (!currentScene->FindGameObjectByEntity(childEntity).IsActive()) continue;
                                ++itemCount;
                            }
                        }
                    }
                }
                else
                {
                    if (!listBox.itemTemplate.typeface) continue;
                }
                sk_sp<SkImage> bgImage = (listBox.backgroundImageTexture && listBox.backgroundImageTexture->getImage())
                                             ? listBox.backgroundImageTexture->getImage()
                                             : nullptr;
                renderables.emplace_back(Renderable{
                    .entityId = entity,
                    .zIndex = listBox.zIndex,
                    .sortKey = getSortKey(entity),
                    .transform = transform,
                    .data = RawListBoxRenderData{
                        .rect = listBox.rect,
                        .roundness = listBox.roundness,
                        .itemCount = itemCount,
                        .items = useContainer ? std::vector<std::string>{} : listBox.items,
                        .selectedIndices = listBox.selectedIndices,
                        .hoveredIndex = listBox.hoveredIndex,
                        .scrollOffset = listBox.scrollOffset,
                        .visibleItemCount = listBox.visibleItemCount,
                        .layout = listBox.layout,
                        .itemSpacing = listBox.itemSpacing,
                        .maxItemsPerRow = listBox.maxItemsPerRow,
                        .maxItemsPerColumn = listBox.maxItemsPerColumn,
                        .useContainer = useContainer,
                        .enableVerticalScrollbar = listBox.enableVerticalScrollbar,
                        .verticalScrollbarAutoHide = listBox.verticalScrollbarAutoHide,
                        .enableHorizontalScrollbar = listBox.enableHorizontalScrollbar,
                        .horizontalScrollbarAutoHide = listBox.horizontalScrollbarAutoHide,
                        .scrollbarThickness = listBox.scrollbarThickness,
                        .itemTemplate = listBox.itemTemplate,
                        .backgroundColor = listBox.backgroundColor,
                        .itemColor = listBox.itemColor,
                        .hoverColor = listBox.hoverColor,
                        .selectedColor = listBox.selectedColor,
                        .disabledColor = listBox.disabledColor,
                        .scrollbarTrackColor = listBox.scrollbarTrackColor,
                        .scrollbarThumbColor = listBox.scrollbarThumbColor,
                        .backgroundImage = bgImage
                    }
                });
            }
        }
    }
    if (!renderables.empty())
    {
        std::ranges::stable_sort(renderables, [](const Renderable& a, const Renderable& b)
        {
            return static_cast<uint32_t>(a.entityId) < static_cast<uint32_t>(b.entityId);
        });
    }
    RenderableManager::GetInstance().SubmitFrame(std::move(renderables));
}
