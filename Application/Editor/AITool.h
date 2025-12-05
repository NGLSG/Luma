#ifndef LUMAENGINE_AITOOL_H
#define LUMAENGINE_AITOOL_H
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "EditorContext.h"
struct AIToolParameter
{
    std::string name; 
    std::string type; 
    std::string description; 
    bool isRequired; 
};
struct AITool
{
    std::string name; 
    std::string description; 
    std::vector<AIToolParameter> parameters; 
    std::function<nlohmann::json(EditorContext&, const nlohmann::json&)> execute;
};
class AIToolRegistry : public LazySingleton<AIToolRegistry>
{
public:
    void RegisterTool(AITool tool);
    const AITool* GetTool(const std::string& name) const;
    nlohmann::json GetToolsManifestAsJson() const;
private:
    std::unordered_map<std::string, AITool> m_tools; 
};
#endif
