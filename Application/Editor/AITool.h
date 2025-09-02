#ifndef LUMAENGINE_AITOOL_H
#define LUMAENGINE_AITOOL_H
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "EditorContext.h"

/**
 * @brief 表示一个AI工具的参数。
 */
struct AIToolParameter
{
    std::string name; ///< 参数的名称。
    std::string type; ///< 参数的数据类型（例如："string", "number", "boolean"）。
    std::string description; ///< 参数的描述。
    bool isRequired; ///< 指示该参数是否为必需。
};

/**
 * @brief 表示一个AI工具的定义。
 * 包含工具的名称、描述、参数列表以及执行该工具的函数。
 */
struct AITool
{
    std::string name; ///< 工具的名称。
    std::string description; ///< 工具的描述。
    std::vector<AIToolParameter> parameters; ///< 工具所需的参数列表。

    /**
     * @brief 执行AI工具的函数。
     * 该函数接收一个编辑器上下文和一个JSON格式的参数对象，并返回一个JSON结果。
     */
    std::function<nlohmann::json(EditorContext&, const nlohmann::json&)> execute;
};

/**
 * @brief AI工具注册表，用于管理和提供AI工具。
 * 这是一个单例类，负责注册、查找和提供AI工具的清单。
 */
class AIToolRegistry : public LazySingleton<AIToolRegistry>
{
public:
    /**
     * @brief 注册一个AI工具。
     * @param tool 要注册的AITool对象。
     */
    void RegisterTool(AITool tool);

    /**
     * @brief 根据名称获取一个AI工具。
     * @param name 要查找的工具的名称。
     * @return 指向找到的AITool对象的指针，如果未找到则返回nullptr。
     */
    const AITool* GetTool(const std::string& name) const;

    /**
     * @brief 获取所有已注册工具的清单，以JSON格式表示。
     * @return 包含所有工具清单的nlohmann::json对象。
     */
    nlohmann::json GetToolsManifestAsJson() const;

private:
    std::unordered_map<std::string, AITool> m_tools; ///< 存储所有已注册AI工具的映射，键为工具名称。
};

#endif