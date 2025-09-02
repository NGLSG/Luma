#include "ScriptComponent.h"
#include "../Application/SceneManager.h"
#include "Resources/RuntimeAsset/RuntimeGameObject.h"
#include <imgui.h>
#include <format>

#include "ScriptMetadataRegistry.h"

namespace CustomDrawing
{
    YAML::Node PropertyValueFactory::CreateDefaultValue(const std::string& typeName, const std::string& defaultValue)
    {
        if (!defaultValue.empty() && defaultValue != "null")
        {
            try
            {
                if (typeName == "float" || typeName == "System.Single")
                {
                    return YAML::Node(std::stof(defaultValue));
                }
                else if (typeName == "int" || typeName == "System.Int32")
                {
                    return YAML::Node(std::stoi(defaultValue));
                }
                else if (typeName == "bool" || typeName == "System.Boolean")
                {
                    return YAML::Node(defaultValue == "true");
                }
                else if (typeName == "string" || typeName == "System.String")
                {
                    return YAML::Node(defaultValue);
                }
            }
            catch (...)
            {
            }
        }


        if (typeName == "float" || typeName == "System.Single")
        {
            return YAML::Node(0.0f);
        }
        else if (typeName == "int" || typeName == "System.Int32")
        {
            return YAML::Node(0);
        }
        else if (typeName == "bool" || typeName == "System.Boolean")
        {
            return YAML::Node(false);
        }
        else if (typeName == "string" || typeName == "System.String")
        {
            return YAML::Node(std::string(""));
        }
        else if (typeName.find("AssetHandle") != std::string::npos ||
            typeName.find("Asset") != std::string::npos)
        {
            YAML::Node assetNode;
            assetNode["guid"] = Guid::Invalid();
            assetNode["type"] = 0;
            return assetNode;
        }
        else if (typeName.find("LumaEvent") != std::string::npos)
        {
            return YAML::Node();
        }
        else
        {
            return YAML::Node();
        }
    }

    bool PropertyValueFactory::IsLumaEventType(const std::string& typeName)
    {
        return typeName.find("LumaEvent") != std::string::npos;
    }


    bool PropertyDrawer::DrawProperty(const std::string& label, const std::string& typeName,
                                      YAML::Node& valueNode, const UIDrawData& drawData)
    {
        if (typeName == "float" || typeName == "System.Single")
        {
            float value = valueNode.as<float>(0.0f);
            if (WidgetDrawer<float>::Draw(label, value, drawData))
            {
                valueNode = value;
                return true;
            }
        }
        else if (typeName == "int" || typeName == "System.Int32")
        {
            int value = valueNode.as<int>(0);
            if (WidgetDrawer<int>::Draw(label, value, drawData))
            {
                valueNode = value;
                return true;
            }
        }
        else if (typeName == "bool" || typeName == "System.Boolean")
        {
            bool value = valueNode.as<bool>(false);
            if (WidgetDrawer<bool>::Draw(label, value, drawData))
            {
                valueNode = value;
                return true;
            }
        }
        else if (typeName == "string" || typeName == "System.String")
        {
            std::string value = valueNode.as<std::string>("");
            if (WidgetDrawer<std::string>::Draw(label, value, drawData))
            {
                valueNode = value;
                return true;
            }
        }
        else if (typeName.find("AssetHandle") != std::string::npos ||
            typeName.find("Asset") != std::string::npos)
        {
            AssetHandle assetHandle;
            if (valueNode.IsMap())
            {
                assetHandle = valueNode.as<AssetHandle>();
            }

            if (WidgetDrawer<AssetHandle>::Draw(label, assetHandle, drawData))
            {
                valueNode = assetHandle;
                return true;
            }
        }
        else if (ScriptMetadataRegistry::GetInstance().GetMetadata(typeName).Valid())
        {
            Guid guid = valueNode.as<Guid>(Guid());
            if (WidgetDrawer<Guid>::Draw(label, guid, drawData))
            {
                valueNode = guid.ToString();
                return true;
            }
        }
        else
        {
            ImGui::Text("%s: [不支持的类型: %s]", label.c_str(), typeName.c_str());
        }

        return false;
    }


    bool EventDrawer::DrawEvent(const std::string& eventName, const std::string& eventSignature,
                                std::vector<ECS::SerializableEventTarget>& targets, const UIDrawData& drawData)
    {
        bool changed = false;
        std::string headerLabel = std::format("{}({})", eventName, eventSignature.empty() ? "void" : eventSignature);
        if (ImGui::TreeNodeEx(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("监听器数量: %zu", targets.size());
            if (ImGui::Button("添加监听器"))
            {
                targets.emplace_back();
                changed = true;
                drawData.onValueChanged.Invoke();
            }
            ImGui::Separator();
            std::vector<int> indicesToRemove;
            for (size_t i = 0; i < targets.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                std::string listenerLabel = std::format("监听器 {}", i);
                if (ImGui::TreeNodeEx(listenerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (WidgetDrawer<Guid>::Draw("目标实体", targets[i].targetEntityGuid, drawData))
                    {
                        changed = true;
                        targets[i].targetComponentName = "ScriptComponent";
                        targets[i].targetMethodName.clear();
                        drawData.onValueChanged.Invoke();
                    }
                    ImGui::Text("组件名称:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "ScriptComponent");
                    targets[i].targetComponentName = "ScriptComponent";
                    ImGui::Text("方法名称:");
                    ImGui::SameLine();
                    auto availableMethods = ScriptMetadataHelper::GetAvailableMethods(
                        targets[i].targetEntityGuid, eventSignature);
                    std::string currentMethodDisplay = targets[i].targetMethodName;
                    if (!targets[i].targetMethodName.empty())
                    {
                        for (const auto& [methodName, signature] : availableMethods)
                        {
                            if (methodName == targets[i].targetMethodName)
                            {
                                currentMethodDisplay = std::format("{}({})", methodName,
                                                                   signature == "void" ? "" : signature);
                                break;
                            }
                        }
                    }
                    ImGui::SetNextItemWidth(200.0f);
                    if (ImGui::BeginCombo("##MethodSelector",
                                          targets[i].targetMethodName.empty() ? "选择方法" : currentMethodDisplay.c_str()))
                    {
                        if (availableMethods.empty()) { ImGui::TextDisabled("无可用方法"); }
                        else
                        {
                            for (const auto& [methodName, signature] : availableMethods)
                            {
                                bool isSelected = (targets[i].targetMethodName == methodName);
                                std::string methodDisplay = std::format(
                                    "{}({})", methodName, signature == "void" ? "" : signature);
                                if (ImGui::Selectable(methodDisplay.c_str(), isSelected))
                                {
                                    targets[i].targetMethodName = methodName;
                                    changed = true;
                                    drawData.onValueChanged.Invoke();
                                }
                                if (isSelected) { ImGui::SetItemDefaultFocus(); }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::Text("方法名: %s", methodName.c_str());
                                    ImGui::Text("参数: %s", signature == "void" ? "无" : signature.c_str());
                                    ImGui::EndTooltip();
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                    RuntimeGameObject targetObject = ScriptMetadataHelper::GetGameObjectByGuid(
                        targets[i].targetEntityGuid);
                    if (targetObject.IsValid()) { ImGui::Text("目标对象: %s", targetObject.GetName().c_str()); }
                    else if (targets[i].targetEntityGuid.Valid())
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "目标对象无效或不存在");
                    }
                    ImGui::Separator();
                    if (ImGui::Button("删除监听器"))
                    {
                        indicesToRemove.push_back(static_cast<int>(i));
                        changed = true;
                        drawData.onValueChanged.Invoke();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            for (int i = static_cast<int>(indicesToRemove.size()) - 1; i >= 0; --i)
            {
                targets.erase(targets.begin() + indicesToRemove[i]);
            }
            ImGui::TreePop();
        }
        return changed;
    }


    void ScriptMetadataHelper::InitializePropertyOverrides(YAML::Node& propertyOverrides,
                                                           const ScriptClassMetadata* metadata)
    {
        if (metadata == nullptr)
            return;

        for (const auto& property : metadata->exportedProperties)
        {
            if (PropertyValueFactory::IsLumaEventType(property.type))
                continue;


            if (!propertyOverrides[property.name])
            {
                propertyOverrides[property.name] =
                    PropertyValueFactory::CreateDefaultValue(property.type, property.defaultValue);
            }
        }
    }

    void ScriptMetadataHelper::InitializeEventLinks(
        std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>>& eventLinks,
        const ScriptClassMetadata* metadata)
    {
        if (metadata == nullptr)
            return;

        for (const auto& property : metadata->exportedProperties)
        {
            if (!PropertyValueFactory::IsLumaEventType(property.type))
                continue;


            if (eventLinks.find(property.name) == eventLinks.end())
            {
                eventLinks[property.name] = std::vector<ECS::SerializableEventTarget>();
            }
        }
    }

    std::vector<std::string> ScriptMetadataHelper::GetMatchingMethods(const ScriptClassMetadata* metadata,
                                                                      const std::string& eventSignature)
    {
        std::vector<std::string> matchingMethods;

        if (metadata == nullptr)
            return matchingMethods;

        for (const auto& method : metadata->publicMethods)
        {
            if (method.returnType != "void")
                continue;


            if (eventSignature == "void" && method.signature == "void")
            {
                matchingMethods.push_back(method.name);
            }
            else if (eventSignature == method.signature)
            {
                matchingMethods.push_back(method.name);
            }
        }

        return matchingMethods;
    }

    const ScriptClassMetadata* ScriptMetadataHelper::GetTargetScriptMetadata(const Guid& targetEntityGuid)
    {
        auto currentScene = SceneManager::GetInstance().GetCurrentScene();
        if (!currentScene)
            return nullptr;


        RuntimeGameObject targetObject = currentScene->FindGameObjectByGuid(targetEntityGuid);
        if (!targetObject.IsValid())
            return nullptr;


        if (!targetObject.HasComponent<ECS::ScriptComponent>())
            return nullptr;


        auto& scriptComponent = targetObject.GetComponent<ECS::ScriptComponent>();
        return scriptComponent.metadata;
    }

    RuntimeGameObject ScriptMetadataHelper::GetGameObjectByGuid(const Guid& targetEntityGuid)
    {
        auto currentScene = SceneManager::GetInstance().GetCurrentScene();
        if (!currentScene)
            return RuntimeGameObject(entt::null, nullptr);


        return currentScene->FindGameObjectByGuid(targetEntityGuid);
    }

    std::vector<std::pair<std::string, std::string>> ScriptMetadataHelper::GetAvailableMethods(
        const Guid& targetEntityGuid, const std::string& eventSignature)
    {
        std::vector<std::pair<std::string, std::string>> availableMethods;

        const ScriptClassMetadata* metadata = GetTargetScriptMetadata(targetEntityGuid);
        if (metadata == nullptr)
            return availableMethods;

        for (const auto& method : metadata->publicMethods)
        {
            if (method.returnType != "void")
                continue;


            if (!eventSignature.empty())
            {
                if (eventSignature == "void" && method.signature == "void")
                {
                    availableMethods.emplace_back(method.name, method.signature);
                }
                else if (eventSignature == method.signature)
                {
                    availableMethods.emplace_back(method.name, method.signature);
                }
            }
            else
            {
                availableMethods.emplace_back(method.name, method.signature);
            }
        }

        return availableMethods;
    }


    const ScriptClassMetadata* getCurrentObjectMetadata(const UIDrawData& drawData)
    {
        if (drawData.SelectedGuids.size() != 1)
            return nullptr;


        const Guid& selectedGuid = drawData.SelectedGuids[0];


        auto currentScene = SceneManager::GetInstance().GetCurrentScene();
        if (!currentScene)
            return nullptr;


        RuntimeGameObject targetObject = currentScene->FindGameObjectByGuid(selectedGuid);
        if (!targetObject.IsValid())
            return nullptr;


        if (!targetObject.HasComponent<ECS::ScriptComponent>())
            return nullptr;


        auto& scriptComponent = targetObject.GetComponent<ECS::ScriptComponent>();
        return scriptComponent.metadata;
    }


    bool WidgetDrawer<YAML::Node>::Draw(const std::string& label, YAML::Node& propertyOverrides,
                                        const UIDrawData& drawData)
    {
        bool changed = false;
        const ScriptClassMetadata* metadata = nullptr;

        if (label == "propertyOverrides" && drawData.SelectedGuids.size() == 1)
        {
            metadata = getCurrentObjectMetadata(drawData);
        }

        if (metadata != nullptr && label == "propertyOverrides")
        {
            std::unordered_set<std::string> validPropertyNames;
            for (const auto& prop : metadata->exportedProperties)
            {
                if (!PropertyValueFactory::IsLumaEventType(prop.type))
                {
                    validPropertyNames.insert(prop.name);
                }
            }
            std::vector<std::string> keysToRemove;
            for (auto it = propertyOverrides.begin(); it != propertyOverrides.end(); ++it)
            {
                const std::string key = it->first.as<std::string>();
                if (validPropertyNames.find(key) == validPropertyNames.end())
                {
                    keysToRemove.push_back(key);
                }
            }
            if (!keysToRemove.empty())
            {
                changed = true;
                for (const auto& key : keysToRemove)
                {
                    propertyOverrides.remove(key);
                }
            }

            ScriptMetadataHelper::InitializePropertyOverrides(propertyOverrides, metadata);

            if (ImGui::TreeNodeEx("覆盖属性", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool hasNonEventProperties = false;
                for (const auto& property : metadata->exportedProperties)
                {
                    if (PropertyValueFactory::IsLumaEventType(property.type)) continue;
                    hasNonEventProperties = true;
                    ImGui::PushID(property.name.c_str());
                    YAML::Node propertyValue = propertyOverrides[property.name];
                    if (PropertyDrawer::DrawProperty(property.name, property.type, propertyValue, drawData))
                    {
                        propertyOverrides[property.name] = propertyValue;
                        changed = true;
                        drawData.onValueChanged.Invoke();
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("类型: %s", property.type.c_str());
                        if (!property.defaultValue.empty()) { ImGui::Text("默认值: %s", property.defaultValue.c_str()); }
                        ImGui::Text("访问性: %s", (property.canGet && property.canSet)
                                                   ? "读/写"
                                                   : property.canGet
                                                   ? "只读"
                                                   : property.canSet
                                                   ? "只写"
                                                   : "无访问");
                        ImGui::EndTooltip();
                    }
                    ImGui::PopID();
                }
                if (!hasNonEventProperties)
                {
                    ImGui::TextDisabled("该脚本没有可覆盖的属性");
                }
                ImGui::TreePop();
            }
        }
        else
        {
            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (propertyOverrides.IsDefined() && !propertyOverrides.IsNull())
                {
                    if (propertyOverrides.IsScalar())
                    {
                        std::string valueStr = propertyOverrides.as<std::string>();
                        char buffer[1024];
#ifdef _WIN32
                        strncpy_s(buffer, sizeof(buffer), valueStr.c_str(), sizeof(buffer) - 1);
#else
                        strncpy(buffer, valueStr.c_str(), sizeof(buffer) - 1);
                        buffer[sizeof(buffer) - 1] = '\0';
#endif
                        if (ImGui::InputText("值", buffer, sizeof(buffer)))
                        {
                            propertyOverrides = std::string(buffer);
                            changed = true;
                            drawData.onValueChanged.Invoke();
                        }
                    }
                    else if (propertyOverrides.IsMap() || propertyOverrides.IsSequence())
                    {
                        if (propertyOverrides.IsMap())
                        {
                            for (auto it = propertyOverrides.begin(); it != propertyOverrides.end(); ++it)
                            {
                                drawYamlNodeRecursive(it->first.as<std::string>(), it->second);
                            }
                        }
                        else
                        {
                            for (size_t i = 0; i < propertyOverrides.size(); ++i)
                            {
                                drawYamlNodeRecursive(std::format("元素 {}", i), propertyOverrides[i]);
                            }
                        }
                    }
                }
                else { ImGui::TextDisabled("空值或未定义"); }
                ImGui::TreePop();
            }
        }
        return changed;
    }

    void WidgetDrawer<YAML::Node>::drawYamlNodeRecursive(const std::string& key, YAML::Node node)
    {
        if (node.IsMap())
        {
            if (ImGui::TreeNode(key.c_str()))
            {
                for (auto it = node.begin(); it != node.end(); ++it)
                {
                    drawYamlNodeRecursive(it->first.as<std::string>(), it->second);
                }
                ImGui::TreePop();
            }
        }
        else if (node.IsSequence())
        {
            if (ImGui::TreeNode(key.c_str()))
            {
                for (size_t i = 0; i < node.size(); ++i) { drawYamlNodeRecursive(std::format("元素 {}", i), node[i]); }
                ImGui::TreePop();
            }
        }
        else if (node.IsScalar())
        {
            ImGui::Text("%s:", key.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", node.as<std::string>().c_str());
        }
        else if (node.IsNull())
        {
            ImGui::Text("%s:", key.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("[空值]");
        }
    }

    bool WidgetDrawer<std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>>>::Draw(
        const std::string& label,
        std::unordered_map<std::string, std::vector<ECS::SerializableEventTarget>>& eventLinks,
        const UIDrawData& drawData)
    {
        bool changed = false;
        const ScriptClassMetadata* metadata = nullptr;
        if (label == "eventLinks" && drawData.SelectedGuids.size() == 1)
        {
            metadata = getCurrentObjectMetadata(drawData);
        }
        if (metadata != nullptr && label == "eventLinks")
        {
            bool hasEventProperties = false;
            for (const auto& property : metadata->exportedProperties)
            {
                if (PropertyValueFactory::IsLumaEventType(property.type))
                {
                    hasEventProperties = true;
                    break;
                }
            }
            if (!hasEventProperties)
            {
                return false;
            }
            ScriptMetadataHelper::InitializeEventLinks(eventLinks, metadata);
            if (ImGui::TreeNodeEx("事件链接", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (const auto& property : metadata->exportedProperties)
                {
                    if (!PropertyValueFactory::IsLumaEventType(property.type)) continue;
                    if (eventLinks.find(property.name) == eventLinks.end())
                    {
                        eventLinks[property.name] = std::vector<ECS::SerializableEventTarget>();
                    }
                    ImGui::PushID(property.name.c_str());
                    if (EventDrawer::DrawEvent(property.name, property.eventSignature, eventLinks[property.name],
                                               drawData))
                    {
                        changed = true;
                    }
                    ImGui::PopID();
                    ImGui::Spacing();
                }
                ImGui::TreePop();
            }
        }
        else
        {
            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (eventLinks.empty()) { ImGui::TextDisabled("无事件链接"); }
                else
                {
                    for (auto& [eventName, targets] : eventLinks)
                    {
                        ImGui::PushID(eventName.c_str());
                        if (EventDrawer::DrawEvent(eventName, "", targets, drawData)) { changed = true; }
                        ImGui::PopID();
                        ImGui::Spacing();
                    }
                }
                ImGui::TreePop();
            }
        }
        return changed;
    }

    bool WidgetDrawer<ECS::SerializableEventTarget>::Draw(const std::string& label,
                                                          ECS::SerializableEventTarget& target,
                                                          const UIDrawData& drawData)
    {
        bool changed = false;
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (WidgetDrawer<Guid>::Draw("目标实体", target.targetEntityGuid, drawData))
            {
                changed = true;
                target.targetComponentName = "ScriptComponent";
                target.targetMethodName.clear();
                drawData.onValueChanged.Invoke();
            }
            ImGui::Text("组件名称:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "ScriptComponent");
            target.targetComponentName = "ScriptComponent";
            ImGui::Text("方法名称:");
            ImGui::SameLine();
            auto availableMethods = ScriptMetadataHelper::GetAvailableMethods(target.targetEntityGuid);
            std::string currentMethodDisplay = target.targetMethodName;
            if (!target.targetMethodName.empty())
            {
                for (const auto& [methodName, signature] : availableMethods)
                {
                    if (methodName == target.targetMethodName)
                    {
                        currentMethodDisplay = std::format("{}({})", methodName, signature == "void" ? "" : signature);
                        break;
                    }
                }
            }
            if (ImGui::BeginCombo("##MethodSelector",
                                  target.targetMethodName.empty() ? "选择方法" : currentMethodDisplay.c_str()))
            {
                if (availableMethods.empty()) { ImGui::TextDisabled("无可用方法"); }
                else
                {
                    for (const auto& [methodName, signature] : availableMethods)
                    {
                        bool isSelected = (target.targetMethodName == methodName);
                        std::string methodDisplay = std::format("{}({})", methodName,
                                                                signature == "void" ? "" : signature);
                        if (ImGui::Selectable(methodDisplay.c_str(), isSelected))
                        {
                            target.targetMethodName = methodName;
                            changed = true;
                            drawData.onValueChanged.Invoke();
                        }
                        if (isSelected) { ImGui::SetItemDefaultFocus(); }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text("方法名: %s", methodName.c_str());
                            ImGui::Text("参数: %s", signature == "void" ? "无" : signature.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                }
                ImGui::EndCombo();
            }
            RuntimeGameObject targetObject = ScriptMetadataHelper::GetGameObjectByGuid(target.targetEntityGuid);
            if (targetObject.IsValid()) { ImGui::Text("目标对象: %s", targetObject.GetName().c_str()); }
            else if (target.targetEntityGuid.Valid())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "目标对象无效或不存在");
            }
            ImGui::TreePop();
        }
        return changed;
    }

    bool WidgetDrawer<ScriptPropertyMetadata>::Draw(const std::string& label, ScriptPropertyMetadata& property,
                                                    const UIDrawData& drawData)
    {
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("名称:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "%s", property.name.c_str());
            ImGui::Text("类型:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", property.type.c_str());
            if (!property.defaultValue.empty())
            {
                ImGui::Text("默认值:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "%s", property.defaultValue.c_str());
            }
            if (!property.eventSignature.empty())
            {
                ImGui::Text("事件签名:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 1.0f, 1.0f), "%s", property.eventSignature.c_str());
            }
            ImGui::Text("访问性:");
            ImGui::SameLine();
            if (property.canGet && property.canSet) { ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "读/写"); }
            else if (property.canGet) { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "只读"); }
            else if (property.canSet) { ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.8f, 1.0f), "只写"); }
            else { ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "无访问"); }
            ImGui::TreePop();
        }
        return false;
    }

    bool WidgetDrawer<ScriptMethodMetadata>::Draw(const std::string& label, ScriptMethodMetadata& method,
                                                  const UIDrawData& drawData)
    {
        if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("名称:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "%s", method.name.c_str());
            ImGui::Text("返回类型:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", method.returnType.c_str());
            ImGui::Text("签名:");
            ImGui::SameLine();
            if (method.signature == "void") { ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "()"); }
            else { ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "(%s)", method.signature.c_str()); }
            ImGui::TreePop();
        }
        return false;
    }

    bool WidgetDrawer<ScriptClassMetadata>::Draw(const std::string& label, ScriptClassMetadata& metadata,
                                                 const UIDrawData& drawData)
    {
        if (ImGui::TreeNodeEx("元信息", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("类名:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.8f, 1.0f), "%s", metadata.name.c_str());
            ImGui::Text("完整名称:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", metadata.fullName.c_str());
            ImGui::Text("命名空间:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.8f, 1.0f, 1.0f, 1.0f), "%s", metadata.nspace.c_str());
            ImGui::Separator();

            if (!metadata.exportedProperties.empty())
            {
                if (ImGui::TreeNodeEx("导出属性", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (size_t i = 0; i < metadata.exportedProperties.size(); ++i)
                    {
                        std::string propertyLabel = std::format("属性 [{}]", i);
                        WidgetDrawer<ScriptPropertyMetadata>::Draw(propertyLabel, metadata.exportedProperties[i],
                                                                   drawData);
                    }
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::TextDisabled("无导出属性");
            }

            if (!metadata.publicMethods.empty())
            {
                if (ImGui::TreeNodeEx("公共方法", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (size_t i = 0; i < metadata.publicMethods.size(); ++i)
                    {
                        std::string methodLabel = std::format("方法 [{}]", i);
                        WidgetDrawer<ScriptMethodMetadata>::Draw(methodLabel, metadata.publicMethods[i], drawData);
                    }
                    ImGui::TreePop();
                }
            }
            else
            {
                ImGui::TextDisabled("无公共方法");
            }
            ImGui::TreePop();
        }
        return false;
    }

    bool WidgetDrawer<const ScriptClassMetadata*>::Draw(const std::string& label,
                                                        const ScriptClassMetadata*& metadataPtr,
                                                        const UIDrawData& drawData)
    {
        if (metadataPtr == nullptr)
        {
            ImGui::TextDisabled("%s: [无元数据可用]", label.c_str());
            return false;
        }
        ScriptClassMetadata metadata = *metadataPtr;
        return WidgetDrawer<ScriptClassMetadata>::Draw(label, metadata, drawData);
    }
}
