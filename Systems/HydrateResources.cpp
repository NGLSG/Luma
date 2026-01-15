#include "HydrateResources.h"

#include "ColliderComponent.h"
#include "GraphicsBackend.h"
#include "MaterialData.h"
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
#include "../Resources/Loaders/ShaderLoader.h"
#include "../Utils/BuiltinShaders.h"
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
            if (registry.all_of<ECS::ToggleButtonComponent>(entity)) OnToggleButtonUpdated(registry, entity);
            if (registry.all_of<ECS::RadioButtonComponent>(entity)) OnRadioButtonUpdated(registry, entity);
            if (registry.all_of<ECS::CheckBoxComponent>(entity)) OnCheckBoxUpdated(registry, entity);
            if (registry.all_of<ECS::SliderComponent>(entity)) OnSliderUpdated(registry, entity);
            if (registry.all_of<ECS::ComboBoxComponent>(entity)) OnComboBoxUpdated(registry, entity);
            if (registry.all_of<ECS::ExpanderComponent>(entity)) OnExpanderUpdated(registry, entity);
            if (registry.all_of<ECS::ProgressBarComponent>(entity)) OnProgressBarUpdated(registry, entity);
            if (registry.all_of<ECS::TabControlComponent>(entity)) OnTabControlUpdated(registry, entity);
            if (registry.all_of<ECS::ListBoxComponent>(entity)) OnListBoxUpdated(registry, entity);
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


        m_listeners.push_back(EventBus::GetInstance().Subscribe<AssetUpdatedEvent>(
            [this](const AssetUpdatedEvent& event)
            {
                auto currentScene = SceneManager::GetInstance().GetCurrentScene();
                if (!currentScene) return;

                auto& registry = currentScene->GetRegistry();


                if (event.assetType == AssetType::Texture)
                {
                    auto spriteView = registry.view<ECS::SpriteComponent>();
                    for (auto entity : spriteView)
                    {
                        auto& sprite = spriteView.get<ECS::SpriteComponent>(entity);
                        if (sprite.textureHandle.assetGuid == event.guid ||
                            sprite.emissionMapHandle.assetGuid == event.guid)
                        {
                            OnSpriteUpdated(registry, entity);
                        }
                    }


                    auto buttonView = registry.view<ECS::ButtonComponent>();
                    for (auto entity : buttonView)
                    {
                        auto& button = buttonView.get<ECS::ButtonComponent>(entity);
                        if (button.backgroundImage.assetGuid == event.guid)
                        {
                            OnButtonUpdated(registry, entity);
                        }
                    }

                    auto inputTextView = registry.view<ECS::InputTextComponent>();
                    for (auto entity : inputTextView)
                    {
                        auto& inputText = inputTextView.get<ECS::InputTextComponent>(entity);
                        if (inputText.backgroundImage.assetGuid == event.guid)
                        {
                            OnInputTextUpdated(registry, entity);
                        }
                    }

                    auto toggleButtonView = registry.view<ECS::ToggleButtonComponent>();
                    for (auto entity : toggleButtonView)
                    {
                        OnToggleButtonUpdated(registry, entity);
                    }

                    auto radioButtonView = registry.view<ECS::RadioButtonComponent>();
                    for (auto entity : radioButtonView)
                    {
                        OnRadioButtonUpdated(registry, entity);
                    }

                    auto checkBoxView = registry.view<ECS::CheckBoxComponent>();
                    for (auto entity : checkBoxView)
                    {
                        OnCheckBoxUpdated(registry, entity);
                    }

                    auto sliderView = registry.view<ECS::SliderComponent>();
                    for (auto entity : sliderView)
                    {
                        OnSliderUpdated(registry, entity);
                    }

                    auto comboBoxView = registry.view<ECS::ComboBoxComponent>();
                    for (auto entity : comboBoxView)
                    {
                        OnComboBoxUpdated(registry, entity);
                    }

                    auto expanderView = registry.view<ECS::ExpanderComponent>();
                    for (auto entity : expanderView)
                    {
                        OnExpanderUpdated(registry, entity);
                    }

                    auto progressBarView = registry.view<ECS::ProgressBarComponent>();
                    for (auto entity : progressBarView)
                    {
                        OnProgressBarUpdated(registry, entity);
                    }

                    auto tabControlView = registry.view<ECS::TabControlComponent>();
                    for (auto entity : tabControlView)
                    {
                        OnTabControlUpdated(registry, entity);
                    }

                    auto listBoxView = registry.view<ECS::ListBoxComponent>();
                    for (auto entity : listBoxView)
                    {
                        OnListBoxUpdated(registry, entity);
                    }
                }
                else if (event.assetType == AssetType::Material)
                {
                    auto spriteView = registry.view<ECS::SpriteComponent>();
                    for (auto entity : spriteView)
                    {
                        auto& sprite = spriteView.get<ECS::SpriteComponent>(entity);
                        if (sprite.materialHandle.assetGuid == event.guid)
                        {
                            OnSpriteUpdated(registry, entity);
                        }
                    }
                }
                else if (event.assetType == AssetType::Font)
                {
                    auto textView = registry.view<ECS::TextComponent>();
                    for (auto entity : textView)
                    {
                        auto& text = textView.get<ECS::TextComponent>(entity);
                        if (text.fontHandle.assetGuid == event.guid)
                        {
                            OnTextUpdated(registry, entity);
                        }
                    }

                    auto inputTextView = registry.view<ECS::InputTextComponent>();
                    for (auto entity : inputTextView)
                    {
                        auto& inputText = inputTextView.get<ECS::InputTextComponent>(entity);
                        if (inputText.text.fontHandle.assetGuid == event.guid ||
                            inputText.placeholder.fontHandle.assetGuid == event.guid)
                        {
                            OnInputTextUpdated(registry, entity);
                        }
                    }
                }

                LogInfo("资产已更新，场景中的相关组件已刷新: {}", event.guid.ToString());
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


        if (sprite.materialHandle.Valid() && (!sprite.wgslMaterial || sprite.lastMaterialHandle != sprite.
            materialHandle))
        {
            auto nutContext = m_context->graphicsBackend->GetNutContext();
            
            // 获取材质定义
            auto materialMeta = AssetManager::GetInstance().GetMetadata(sprite.materialHandle.assetGuid);
            if (!materialMeta)
            {
                LogError("Failed to get material metadata: {}", sprite.materialHandle.assetGuid.ToString());
                return;
            }
            
            auto materialDef = materialMeta->importerSettings.as<Data::MaterialDefinition>();
            
            // 获取shader语言，支持内建shader
            Data::ShaderLanguage shaderLan = Data::ShaderLanguage::WGSL;
            if (BuiltinShaders::IsBuiltinShaderGuid(materialDef.shaderHandle.assetGuid))
            {
                // 内建shader都是WGSL
                shaderLan = Data::ShaderLanguage::WGSL;
            }
            else
            {
                auto shaderMeta = AssetManager::GetInstance().GetMetadata(materialDef.shaderHandle.assetGuid);
                if (shaderMeta && shaderMeta->importerSettings.IsDefined())
                {
                    shaderLan = shaderMeta->importerSettings.as<Data::ShaderData>().language;
                }
            }
            
            if (nutContext && shaderLan == Data::ShaderLanguage::WGSL)
            {
                sprite.wgslMaterial = materialLoader.LoadWGSLMaterial(sprite.materialHandle.assetGuid, nutContext);
                if (!sprite.wgslMaterial)
                {
                    LogError("Failed to load WGSL material with GUID: {}", sprite.materialHandle.assetGuid.ToString());


                    sprite.material = materialLoader.LoadAsset(sprite.materialHandle.assetGuid);
                    if (!sprite.material)
                    {
                        LogError("Failed to load fallback SkSL material with GUID: {}",
                                 sprite.materialHandle.assetGuid.ToString());
                    }
                }
                else
                {
                    sprite.material.reset();
                }
            }
            else
            {
                LogWarn("NutContext not available, loading SkSL material as fallback");
                sprite.material = materialLoader.LoadAsset(sprite.materialHandle.assetGuid);
                if (!sprite.material)
                {
                    LogError("Failed to load material with GUID: {}", sprite.materialHandle.assetGuid.ToString());
                }
            }
            sprite.lastMaterialHandle = sprite.materialHandle;
        }
        else if (!sprite.materialHandle.Valid())
        {
            sprite.material.reset();
            sprite.wgslMaterial.reset();
        }

        // 加载自发光贴图
        // Feature: 2d-lighting-enhancement
        // Requirements: 4.1
        if (sprite.emissionMapHandle.Valid() && (!sprite.emissionMapImage || sprite.lastEmissionMapHandle != sprite.emissionMapHandle))
        {
            sprite.emissionMapImage = textureLoader.LoadAsset(sprite.emissionMapHandle.assetGuid);
            if (!sprite.emissionMapImage)
            {
                LogError("Failed to load emission map with GUID: {}", sprite.emissionMapHandle.assetGuid.ToString());
            }
            sprite.lastEmissionMapHandle = sprite.emissionMapHandle;
        }
        else if (!sprite.emissionMapHandle.Valid())
        {
            sprite.emissionMapImage.reset();
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


    void HydrateResources::OnToggleButtonUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;

        auto& toggle = registry.get<ECS::ToggleButtonComponent>(entity);
        TextureLoader textureLoader(*m_context->graphicsBackend);

        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load ToggleButton {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(toggle.backgroundImage, toggle.backgroundImageTexture, "backgroundImage");
    }

    void HydrateResources::OnRadioButtonUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& radio = registry.get<ECS::RadioButtonComponent>(entity);

        {
            FontLoader fontLoader;
            auto ensureTypeface = [&](ECS::TextComponent& text, const char* fieldName)
            {
                if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
                {
                    text.typeface = fontLoader.LoadAsset(text.fontHandle.assetGuid);
                    if (!text.typeface)
                    {
                        LogError("Failed to load RadioButton {} font with GUID: {}", fieldName,
                                 text.fontHandle.assetGuid.ToString());
                    }
                    text.lastFontHandle = text.fontHandle;
                }
                else if (!text.fontHandle.Valid())
                {
                    text.typeface.reset();
                    text.lastFontHandle = {};
                }
            };

            ensureTypeface(radio.label, "label");
        }

        if (!m_context || !m_context->graphicsBackend) return;
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load RadioButton {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(radio.backgroundImage, radio.backgroundImageTexture, "backgroundImage");
        loadTexture(radio.selectionImage, radio.selectionImageTexture, "selectionImage");
    }

    void HydrateResources::OnCheckBoxUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& checkbox = registry.get<ECS::CheckBoxComponent>(entity);

        {
            FontLoader fontLoader;
            auto ensureTypeface = [&](ECS::TextComponent& text, const char* fieldName)
            {
                if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
                {
                    text.typeface = fontLoader.LoadAsset(text.fontHandle.assetGuid);
                    if (!text.typeface)
                    {
                        LogError("Failed to load CheckBox {} font with GUID: {}", fieldName,
                                 text.fontHandle.assetGuid.ToString());
                    }
                    text.lastFontHandle = text.fontHandle;
                }
                else if (!text.fontHandle.Valid())
                {
                    text.typeface.reset();
                    text.lastFontHandle = {};
                }
            };

            ensureTypeface(checkbox.label, "label");
        }

        if (!m_context || !m_context->graphicsBackend) return;
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load CheckBox {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(checkbox.backgroundImage, checkbox.backgroundImageTexture, "backgroundImage");
        loadTexture(checkbox.checkmarkImage, checkbox.checkmarkImageTexture, "checkmarkImage");
    }

    void HydrateResources::OnSliderUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;

        auto& slider = registry.get<ECS::SliderComponent>(entity);
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load Slider {} texture with GUID: {}", fieldName, handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(slider.trackImage, slider.trackImageTexture, "trackImage");
        loadTexture(slider.fillImage, slider.fillImageTexture, "fillImage");
        loadTexture(slider.thumbImage, slider.thumbImageTexture, "thumbImage");
    }

    void HydrateResources::OnComboBoxUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& combo = registry.get<ECS::ComboBoxComponent>(entity);

        {
            FontLoader fontLoader;
            auto ensureTypeface = [&](ECS::TextComponent& text, const char* fieldName)
            {
                if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
                {
                    text.typeface = fontLoader.LoadAsset(text.fontHandle.assetGuid);
                    if (!text.typeface)
                    {
                        LogError("Failed to load ComboBox {} font with GUID: {}", fieldName,
                                 text.fontHandle.assetGuid.ToString());
                    }
                    text.lastFontHandle = text.fontHandle;
                }
                else if (!text.fontHandle.Valid())
                {
                    text.typeface.reset();
                    text.lastFontHandle = {};
                }
            };

            ensureTypeface(combo.displayText, "displayText");
        }

        if (!m_context || !m_context->graphicsBackend) return;
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load ComboBox {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(combo.backgroundImage, combo.backgroundImageTexture, "backgroundImage");
        loadTexture(combo.dropdownIcon, combo.dropdownIconTexture, "dropdownIcon");
    }

    void HydrateResources::OnExpanderUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& expander = registry.get<ECS::ExpanderComponent>(entity);

        {
            FontLoader fontLoader;
            auto ensureTypeface = [&](ECS::TextComponent& text, const char* fieldName)
            {
                if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
                {
                    text.typeface = fontLoader.LoadAsset(text.fontHandle.assetGuid);
                    if (!text.typeface)
                    {
                        LogError("Failed to load Expander {} font with GUID: {}", fieldName,
                                 text.fontHandle.assetGuid.ToString());
                    }
                    text.lastFontHandle = text.fontHandle;
                }
                else if (!text.fontHandle.Valid())
                {
                    text.typeface.reset();
                    text.lastFontHandle = {};
                }
            };

            ensureTypeface(expander.header, "header");
        }

        if (!m_context || !m_context->graphicsBackend) return;
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load Expander {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(expander.backgroundImage, expander.backgroundImageTexture, "backgroundImage");
    }

    void HydrateResources::OnProgressBarUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;

        auto& progress = registry.get<ECS::ProgressBarComponent>(entity);
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load ProgressBar {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(progress.backgroundImage, progress.backgroundImageTexture, "backgroundImage");
        loadTexture(progress.fillImage, progress.fillImageTexture, "fillImage");
    }

    void HydrateResources::OnTabControlUpdated(entt::registry& registry, entt::entity entity)
    {
        if (!m_context || !m_context->graphicsBackend) return;

        auto& tabControl = registry.get<ECS::TabControlComponent>(entity);
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load TabControl {} texture with GUID: {}", fieldName,
                             handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(tabControl.backgroundImage, tabControl.backgroundImageTexture, "backgroundImage");
        loadTexture(tabControl.tabBackgroundImage, tabControl.tabBackgroundImageTexture, "tabBackgroundImage");
    }

    void HydrateResources::OnListBoxUpdated(entt::registry& registry, entt::entity entity)
    {
        auto& listBox = registry.get<ECS::ListBoxComponent>(entity);

        {
            FontLoader fontLoader;
            auto ensureTypeface = [&](ECS::TextComponent& text, const char* fieldName)
            {
                if (text.fontHandle.Valid() && (!text.typeface || text.lastFontHandle != text.fontHandle))
                {
                    text.typeface = fontLoader.LoadAsset(text.fontHandle.assetGuid);
                    if (!text.typeface)
                    {
                        LogError("Failed to load ListBox {} font with GUID: {}", fieldName,
                                 text.fontHandle.assetGuid.ToString());
                    }
                    text.lastFontHandle = text.fontHandle;
                }
                else if (!text.fontHandle.Valid())
                {
                    text.typeface.reset();
                    text.lastFontHandle = {};
                }
            };

            if (!listBox.itemsContainerGuid.Valid())
            {
                ensureTypeface(listBox.itemTemplate, "itemTemplate");
            }
            else
            {
                listBox.itemTemplate.typeface.reset();
                listBox.itemTemplate.lastFontHandle = {};
            }
        }

        if (!m_context || !m_context->graphicsBackend) return;
        TextureLoader textureLoader(*m_context->graphicsBackend);
        auto loadTexture = [&](const AssetHandle& handle, sk_sp<RuntimeTexture>& target, const char* fieldName)
        {
            if (handle.Valid() && (!target || target->GetSourceGuid() != handle.assetGuid))
            {
                target = textureLoader.LoadAsset(handle.assetGuid);
                if (!target)
                {
                    LogError("Failed to load ListBox {} texture with GUID: {}", fieldName, handle.assetGuid.ToString());
                }
            }
            else if (!handle.Valid())
            {
                target.reset();
            }
        };

        loadTexture(listBox.backgroundImage, listBox.backgroundImageTexture, "backgroundImage");
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

            const float cellWidth = tilemap.cellSize.x;
            const float cellHeight = tilemap.cellSize.y;

            const float shiftX = cellWidth * (-0.5f);
            const float shiftY = cellHeight * (-0.5f);

            if (!tilemap.runtimeTileCache.empty())
            {
                int minX = std::numeric_limits<int>::max();
                int maxX = std::numeric_limits<int>::lowest();
                int minY = std::numeric_limits<int>::max();
                int maxY = std::numeric_limits<int>::lowest();
                for (const auto& [coord, _] : tilemap.runtimeTileCache)
                {
                    minX = std::min(minX, coord.x);
                    maxX = std::max(maxX, coord.x);
                    minY = std::min(minY, coord.y);
                    maxY = std::max(maxY, coord.y);
                }


                for (int y = minY; y <= maxY + 1; ++y)
                {
                    bool running = false;
                    int runStartX = 0;
                    for (int x = minX; x <= maxX; ++x)
                    {
                        bool present = isSolid({x, y}) != isSolid({x, y - 1});
                        if (present)
                        {
                            if (!running)
                            {
                                running = true;
                                runStartX = x;
                            }
                        }
                        else if (running)
                        {
                            int runEndX = x;
                            if (runEndX > runStartX)
                            {
                                std::vector<ECS::Vector2f> chain;
                                chain.emplace_back(runStartX * cellWidth + shiftX, y * cellHeight + shiftY);
                                chain.emplace_back(runEndX * cellWidth + shiftX, y * cellHeight + shiftY);
                                tilemapCollider.generatedChains.push_back(std::move(chain));
                            }
                            running = false;
                        }
                    }
                    if (running)
                    {
                        int runEndX = maxX + 1;
                        if (runEndX > runStartX)
                        {
                            std::vector<ECS::Vector2f> chain;
                            chain.emplace_back(runStartX * cellWidth + shiftX, y * cellHeight + shiftY);
                            chain.emplace_back(runEndX * cellWidth + shiftX, y * cellHeight + shiftY);
                            tilemapCollider.generatedChains.push_back(std::move(chain));
                        }
                    }
                }


                for (int x = minX; x <= maxX + 1; ++x)
                {
                    bool running = false;
                    int runStartY = 0;
                    for (int y = minY; y <= maxY; ++y)
                    {
                        bool present = isSolid({x, y}) != isSolid({x - 1, y});
                        if (present)
                        {
                            if (!running)
                            {
                                running = true;
                                runStartY = y;
                            }
                        }
                        else if (running)
                        {
                            int runEndY = y;
                            if (runEndY > runStartY)
                            {
                                std::vector<ECS::Vector2f> chain;
                                chain.emplace_back(x * cellWidth + shiftX, runStartY * cellHeight + shiftY);
                                chain.emplace_back(x * cellWidth + shiftX, runEndY * cellHeight + shiftY);
                                tilemapCollider.generatedChains.push_back(std::move(chain));
                            }
                            running = false;
                        }
                    }
                    if (running)
                    {
                        int runEndY = maxY + 1;
                        if (runEndY > runStartY)
                        {
                            std::vector<ECS::Vector2f> chain;
                            chain.emplace_back(x * cellWidth + shiftX, runStartY * cellHeight + shiftY);
                            chain.emplace_back(x * cellWidth + shiftX, runEndY * cellHeight + shiftY);
                            tilemapCollider.generatedChains.push_back(std::move(chain));
                        }
                    }
                }
            }

            tilemapCollider.isDirty = true;
        }
    }
}
