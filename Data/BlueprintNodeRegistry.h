#ifndef LUMAENGINE_BLUEPRINTNODEREGISTRY_H
#define LUMAENGINE_BLUEPRINTNODEREGISTRY_H

#include "Utils/LazySingleton.h"
#include "Data/BlueprintData.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <map>

namespace ax::NodeEditor
{
    enum class PinKind;
}

struct BlueprintPinDefinition
{
    std::string Name;
    std::string Type;
    ax::NodeEditor::PinKind Kind;
};

struct BlueprintNodeDefinition
{
    std::string FullName;
    std::string DisplayName;
    std::string Category;
    std::string Description;
    BlueprintNodeType NodeType;

    std::vector<BlueprintPinDefinition> InputPins;
    std::vector<BlueprintPinDefinition> OutputPins;
    std::string DynamicArrayPinName;
};


class BlueprintNodeRegistry : public LazySingleton<BlueprintNodeRegistry>
{
public:
    friend class LazySingleton<BlueprintNodeRegistry>;

    /**
     * @brief 重新注册所有核心节点和脚本节点。
     *
     * 此函数会清空当前所有节点定义，然后重新加载核心节点和所有从脚本元数据中发现的函数节点。
     * 在脚本重新编译后调用此函数，以更新节点列表。
     */
    void RegisterAll();

    void RegisterNode(BlueprintNodeDefinition definition);

    const BlueprintNodeDefinition* GetDefinition(const std::string& fullName) const;

    const std::map<std::string, std::vector<const BlueprintNodeDefinition*>>& GetCategorizedDefinitions() const;

private:
    BlueprintNodeRegistry();
    void RegisterScriptNodes();
    void RegisterCoreNodes();
    void RegisterSDKNodes();

    std::unordered_map<std::string, BlueprintNodeDefinition> m_definitions;

    std::map<std::string, std::vector<const BlueprintNodeDefinition*>> m_categorizedDefinitions;
};

#endif // LUMAENGINE_BLUEPRINTNODEREGISTRY_H