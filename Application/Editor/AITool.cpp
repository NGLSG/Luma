#include "AITool.h"
#include "Utils/Logger.h"
#include <nlohmann/json.hpp>
#include "../Resources/RuntimeAsset/RuntimeScene.h"

void AIToolRegistry::RegisterTool(AITool tool)
{
    if (m_tools.count(tool.name))
    {
        LogWarn("AI 工具 '{}' 已存在，将被新版本覆盖。", tool.name);
    }
    else
    {
        LogInfo("注册 AI 工具: '{}'", tool.name);
    }
    m_tools[tool.name] = std::move(tool);
}

const AITool* AIToolRegistry::GetTool(const std::string& name) const
{
    auto it = m_tools.find(name);
    if (it != m_tools.end())
    {
        return &it->second;
    }
    return nullptr;
}


nlohmann::json AIToolRegistry::GetToolsManifestAsJson() const
{
    nlohmann::json manifest = nlohmann::json::array();

    for (const auto& [name, tool] : m_tools)
    {
        nlohmann::json toolJson;
        toolJson["name"] = tool.name;
        toolJson["description"] = tool.description;

        nlohmann::json paramsJson;
        paramsJson["type"] = "object";

        nlohmann::json propsJson = nlohmann::json::object();
        nlohmann::json requiredParams = nlohmann::json::array();

        for (const auto& param : tool.parameters)
        {
            nlohmann::json paramDetailJson;
            paramDetailJson["type"] = param.type;
            paramDetailJson["description"] = param.description;
            propsJson[param.name] = paramDetailJson;

            if (param.isRequired)
            {
                requiredParams.push_back(param.name);
            }
        }

        paramsJson["properties"] = propsJson;
        if (!requiredParams.empty())
        {
            paramsJson["required"] = requiredParams;
        }

        toolJson["parameters"] = paramsJson;
        manifest.push_back(toolJson);
    }

    return manifest;
}
