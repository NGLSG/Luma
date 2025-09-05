#include "BlueprintImporter.h"

#include "Path.h"
#include "Utils.h"
#include "AIServices/include/Utils.h"
#include "Resources/AssetManager.h"
#include "Data/BlueprintData.h"
#include "Loaders/BlueprintLoader.h"

#include "Data/BlueprintData.h"
#include "BlueprintNodeRegistry.h"
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <set>


namespace BlueprintCSharpCodeGenerator
{
    struct CodeGenContext;
    std::string ResolveValue(CodeGenContext& ctx, uint32_t targetNodeID, const std::string& targetPinName);
    void GenerateExecChain(CodeGenContext& ctx, uint32_t startNodeID, const std::string& startPinName);

    struct CodeGenContext
    {
        const Blueprint& blueprint;
        std::stringstream ss;
        int indent = 0;
        uint32_t tempVarCounter = 0;

        std::map<std::pair<uint32_t, std::string>, std::string> valueCache;
        std::unordered_map<uint32_t, const BlueprintNode*> nodeLookup;
        std::unordered_map<uint32_t, std::vector<const BlueprintLink*>> nodeOutputLinks;

        CodeGenContext(const Blueprint& bp) : blueprint(bp)
        {
            for (const auto& node : bp.Nodes)
            {
                nodeLookup[node.ID] = &node;
            }
            for (const auto& link : bp.Links)
            {
                nodeOutputLinks[link.FromNodeID].push_back(&link);
            }
        }

        void AppendLine(const std::string& line)
        {
            if (!line.empty())
            {
                ss << std::string(indent * 4, ' ') << line << "\n";
            }
            else
            {
                ss << "\n";
            }
        }

        const BlueprintLink* FindLinkFromOutput(uint32_t nodeID, const std::string& pinName)
        {
            if (nodeOutputLinks.count(nodeID))
            {
                for (const auto* link : nodeOutputLinks.at(nodeID))
                {
                    if (link->FromPinName == pinName)
                    {
                        return link;
                    }
                }
            }
            return nullptr;
        }

        const BlueprintLink* FindLinkToInput(uint32_t nodeID, const std::string& pinName)
        {
            for (const auto& link : blueprint.Links)
            {
                if (link.ToNodeID == nodeID && link.ToPinName == pinName)
                {
                    return &link;
                }
            }
            return nullptr;
        }
    };

    
    
    
    std::string SanitizeValue(const std::string& value, const std::string& type)
    {
        if (type == "System.String") return "\"" + value + "\"";
        if (type == "System.Single") return value + "f";
        return value;
    }

    
    
    
    std::string ResolveValue(CodeGenContext& ctx, uint32_t targetNodeID, const std::string& targetPinName)
    {
        const auto* link = ctx.FindLinkToInput(targetNodeID, targetPinName);
        const BlueprintNode* targetNode = ctx.nodeLookup.at(targetNodeID);

        
        if (!link)
        {
            if (targetNode->InputDefaults.count(targetPinName))
            {
                
                return targetNode->InputDefaults.at(targetPinName);
            }
            return "default";
        }

        auto cacheKey = std::make_pair(link->FromNodeID, link->FromPinName);
        if (ctx.valueCache.count(cacheKey))
        {
            return ctx.valueCache.at(cacheKey);
        }

        const BlueprintNode* sourceNode = ctx.nodeLookup.at(link->FromNodeID);
        std::string resultVar;

        switch (sourceNode->Type)
        {
        case BlueprintNodeType::VariableGet:
            resultVar = sourceNode->VariableName;
            break;
        case BlueprintNodeType::FunctionEntry:
            resultVar = link->FromPinName;
            break;
        case BlueprintNodeType::Declaration:
            resultVar = ctx.valueCache.count({sourceNode->ID, "输出变量"})
                            ? ctx.valueCache.at({sourceNode->ID, "输出变量"})
                            : "/* 错误: 变量未声明 */";
            break;
        case BlueprintNodeType::FlowControl:
            {
                std::string fullName = sourceNode->TargetClassFullName + "." + sourceNode->TargetMemberName;
                if (fullName == "Utility.GetSelf")
                {
                    resultVar = "this";
                }
                if (fullName == "Utility.GetEntity")
                {
                    resultVar = "Self";
                }
                
                else if (fullName == "Utility.Input")
                {
                    std::string value = sourceNode->InputDefaults.count("值") ? sourceNode->InputDefaults.at("值") : "";
                    std::string type = sourceNode->InputDefaults.count("类型")
                                           ? sourceNode->InputDefaults.at("类型")
                                           : "System.Object";
                    resultVar = SanitizeValue(value, type);
                }
                else
                {
                    
                    if (ctx.valueCache.count({sourceNode->ID, link->FromPinName}))
                    {
                        resultVar = ctx.valueCache.at({sourceNode->ID, link->FromPinName});
                    }
                    else
                    {
                        resultVar = "default";
                    }
                }
                break;
            }
        default:
            
            if (ctx.valueCache.count({sourceNode->ID, link->FromPinName}))
            {
                resultVar = ctx.valueCache.at({sourceNode->ID, link->FromPinName});
            }
            else
            {
                resultVar = "default";
            }
            break;
        }

        ctx.valueCache[cacheKey] = resultVar;
        return resultVar;
    }

    
    
    
    void GenerateExecChain(CodeGenContext& ctx, uint32_t startNodeID, const std::string& startPinName)
    {
        const auto* currentLink = ctx.FindLinkFromOutput(startNodeID, startPinName);
        if (!currentLink) return;

        uint32_t currentNodeID = currentLink->ToNodeID;
        const BlueprintNode* currentNode = ctx.nodeLookup.at(currentNodeID);

        switch (currentNode->Type)
        {
        case BlueprintNodeType::VariableSet:
            {
                
                std::string varName;
                if (!currentNode->VariableName.empty())
                {
                    varName = currentNode->VariableName;
                }
                else if (currentNode->InputDefaults.count("变量名"))
                {
                    varName = currentNode->InputDefaults.at("变量名");
                }

                if (!varName.empty())
                {
                    std::string value = ResolveValue(ctx, currentNodeID, "值");
                    ctx.AppendLine(varName + " = " + value + ";");
                }
                else
                {
                    ctx.AppendLine("/* 错误: 设置变量节点未指定变量名 */");
                }

                GenerateExecChain(ctx, currentNodeID, "然后");
                break;
            }
        case BlueprintNodeType::FunctionCall:
            {
                
                if (currentNode->TargetClassFullName == "Luma.SDK.Entity")
                {
                    const std::string& member = currentNode->TargetMemberName;
                    if (member == "HasComponent" || member == "GetComponent" || member == "AddComponent" || member ==
                        "SetComponent")
                    {
                        
                        std::string target = ResolveValue(ctx, currentNodeID, "目标");
                        std::string componentType = "object";
                        if (currentNode->InputDefaults.count("组件类型"))
                        {
                            componentType = currentNode->InputDefaults.at("组件类型");
                        }

                        if (componentType.empty() || componentType == "object" || componentType == "选择类型")
                        {
                            ctx.AppendLine("/* 错误: 节点(ID:" + std::to_string(currentNode->ID) + ") 的组件类型未指定 */");
                        }
                        else
                        {
                            if (member == "HasComponent")
                            {
                                std::string tempVar = "temp_" + std::to_string(ctx.tempVarCounter++);
                                ctx.AppendLine(
                                    "bool " + tempVar + " = " + target + ".HasComponent<" + componentType + ">();");
                                ctx.valueCache[{currentNode->ID, "返回值"}] = tempVar;
                            }
                            else if (member == "GetComponent")
                            {
                                std::string tempVar = "temp_" + std::to_string(ctx.tempVarCounter++);
                                ctx.AppendLine(
                                    "var " + tempVar + " = " + target + ".GetComponent<" + componentType + ">();");
                                ctx.valueCache[{currentNode->ID, "返回值"}] = tempVar;
                            }
                            else if (member == "AddComponent")
                            {
                                std::string tempVar = "temp_" + std::to_string(ctx.tempVarCounter++);
                                ctx.AppendLine(
                                    "var " + tempVar + " = " + target + ".AddComponent<" + componentType + ">();");
                                ctx.valueCache[{currentNode->ID, "返回值"}] = tempVar;
                            }
                            else if (member == "SetComponent")
                            {
                                std::string valueToSet = ResolveValue(ctx, currentNodeID, "组件值");
                                ctx.AppendLine(target + ".SetComponent<" + componentType + ">(" + valueToSet + ");");
                            }
                        }

                        
                        GenerateExecChain(ctx, currentNodeID, "然后");
                        break; 
                    }
                }

                
                const auto& funcs = ctx.blueprint.Functions;
                auto it = std::find_if(funcs.begin(), funcs.end(), [&](const BlueprintFunction& func)
                {
                    return func.Name == currentNode->TargetMemberName;
                });

                
                if (it != funcs.end())
                {
                    const auto& func = *it;
                    std::string params;
                    bool first = true;
                    for (const auto& param : func.Parameters)
                    {
                        if (!first) params += ", ";
                        params += ResolveValue(ctx, currentNodeID, param.Name);
                        first = false;
                    }

                    std::string callStmt = currentNode->TargetMemberName + "(" + params + ")";
                    if (func.ReturnType != "void")
                    {
                        std::string tempVar = "temp_" + std::to_string(ctx.tempVarCounter++);
                        ctx.AppendLine(func.ReturnType + " " + tempVar + " = " + callStmt + ";");
                        ctx.valueCache[{currentNode->ID, "返回值"}] = tempVar;
                    }
                    else
                    {
                        ctx.AppendLine(callStmt + ";");
                    }
                }
                
                else
                {
                    std::string funcFullName = currentNode->TargetClassFullName + "." + currentNode->TargetMemberName;
                    const auto* definition = BlueprintNodeRegistry::GetInstance().GetDefinition(funcFullName);

                    if (!definition)
                    {
                        ctx.AppendLine("/* 错误: 找不到函数定义 '" + funcFullName + "' */");
                    }
                    else
                    {
                        bool isNonStatic = false;
                        for (const auto& pin : definition->InputPins)
                        {
                            if (pin.Name == "目标")
                            {
                                isNonStatic = true;
                                break;
                            }
                        }

                        std::string params;
                        bool first = true;
                        for (const auto& pin : definition->InputPins)
                        {
                            if (pin.Type != "Exec" && pin.Name != "目标" && pin.Type != "TemplateType")
                            {
                                if (!first) params += ", ";
                                params += ResolveValue(ctx, currentNodeID, pin.Name);
                                first = false;
                            }
                        }

                        std::string callStmt;
                        if (isNonStatic)
                        {
                            std::string target = ResolveValue(ctx, currentNodeID, "目标");
                            callStmt = (target == "default" || target.empty())
                                           ? "/* 错误: 目标未连接 */"
                                           : target + "." + currentNode->TargetMemberName + "(" + params + ")";
                        }
                        else
                        {
                            callStmt = currentNode->TargetClassFullName + "." + currentNode->TargetMemberName + "(" +
                                params + ")";
                        }

                        std::string returnType = "void";
                        std::string returnPinName;
                        for (const auto& pin : definition->OutputPins)
                        {
                            if (pin.Type != "Exec")
                            {
                                returnType = pin.Type;
                                returnPinName = pin.Name;
                                break;
                            }
                        }

                        if (returnType != "void")
                        {
                            std::string tempVar = "temp_" + std::to_string(ctx.tempVarCounter++);
                            ctx.AppendLine(returnType + " " + tempVar + " = " + callStmt + ";");
                            ctx.valueCache[{currentNode->ID, returnPinName}] = tempVar;
                        }
                        else
                        {
                            ctx.AppendLine(callStmt + ";");
                        }
                    }
                }

                GenerateExecChain(ctx, currentNodeID, "然后");
                break;
            }
        case BlueprintNodeType::FlowControl:
            {
                std::string fullName = currentNode->TargetClassFullName + "." + currentNode->TargetMemberName;
                if (fullName == "FlowControl.If")
                {
                    std::string condition = ResolveValue(ctx, currentNodeID, "条件");
                    if (condition == "default" || condition.empty())
                    {
                        condition = "false";
                    }

                    ctx.AppendLine("if (" + condition + ")");
                    ctx.AppendLine("{");
                    ctx.indent++;
                    GenerateExecChain(ctx, currentNodeID, "为真");
                    ctx.indent--;
                    ctx.AppendLine("}");
                    ctx.AppendLine("else");
                    ctx.AppendLine("{");
                    ctx.indent++;
                    GenerateExecChain(ctx, currentNodeID, "为假");
                    ctx.indent--;
                    ctx.AppendLine("}");
                    GenerateExecChain(ctx, currentNodeID, "然后");
                }
                else if (fullName == "FlowControl.ForLoop")
                {
                    std::string firstIndex = ResolveValue(ctx, currentNodeID, "起始索引");
                    std::string lastIndex = ResolveValue(ctx, currentNodeID, "结束索引");
                    std::string loopVar = "i_" + std::to_string(currentNodeID);
                    ctx.valueCache[{currentNodeID, "当前索引"}] = loopVar;

                    ctx.AppendLine(
                        "for (int " + loopVar + " = " + firstIndex + "; " + loopVar + " <= " + lastIndex + "; " +
                        loopVar + "++)");
                    ctx.AppendLine("{");
                    ctx.indent++;
                    GenerateExecChain(ctx, currentNodeID, "循环体");
                    ctx.indent--;
                    ctx.AppendLine("}");
                    GenerateExecChain(ctx, currentNodeID, "然后");
                }
                else if (fullName == "FlowControl.Return")
                {
                    std::string returnType = "void";
                    if (currentNode->InputDefaults.count("返回类型"))
                    {
                        returnType = currentNode->InputDefaults.at("返回类型");
                    }

                    if (returnType == "void" || returnType.empty())
                    {
                        ctx.AppendLine("return;");
                    }
                    else
                    {
                        std::string valueToReturn = ResolveValue(ctx, currentNodeID, "输入值");
                        ctx.AppendLine("return " + valueToReturn + ";");
                    }
                }
                break;
            }
        case BlueprintNodeType::Declaration:
            {
                std::string varType = currentNode->InputDefaults.count("变量类型")
                                          ? currentNode->InputDefaults.at("变量类型")
                                          : "var";
                std::string varName = currentNode->InputDefaults.count("变量名")
                                          ? currentNode->InputDefaults.at("变量名")
                                          : ("__unnamed_var_" + std::to_string(currentNode->ID));

                if (!varType.empty() && !varName.empty())
                {
                    std::string initialValue = ResolveValue(ctx, currentNodeID, "初始值");
                    std::string declaration = varType + " " + varName;
                    if (initialValue != "default")
                    {
                        declaration += " = " + initialValue;
                    }
                    declaration += ";";
                    ctx.AppendLine(declaration);

                    ctx.valueCache[{currentNodeID, "输出变量"}] = varName;
                }
                GenerateExecChain(ctx, currentNodeID, "然后");
                break;
            }
        default:
            break;
        }
    }

    
    
    
    static std::string Generate(const Blueprint& blueprintData)
    {
        CodeGenContext ctx(blueprintData);

        ctx.ss << "using Luma.SDK;\n";
        ctx.ss << "using System;\n\n";

        ctx.ss << "namespace GameScripts\n";
        ctx.ss << "{\n";
        ctx.indent = 1;

        ctx.AppendLine("public class " + blueprintData.Name + " : " + blueprintData.ParentClass);
        ctx.AppendLine("{");
        ctx.indent++;

        
        for (const auto& var : blueprintData.Variables)
        {
            std::string line = "[Export]\npublic " + var.Type + " " + var.Name;
            if (!var.DefaultValue.empty())
            {
                line += " = " + SanitizeValue(var.DefaultValue, var.Type);
            }
            line += ";";
            ctx.AppendLine(line);
        }
        if (!blueprintData.Variables.empty())
        {
            ctx.AppendLine("");
        }

        
        for (const auto& node : blueprintData.Nodes)
        {
            if (node.Type == BlueprintNodeType::Event)
            {
                std::string funcFullName = node.TargetClassFullName + "." + node.TargetMemberName;
                const auto* definition = BlueprintNodeRegistry::GetInstance().GetDefinition(funcFullName);
                if (!definition) continue;

                std::string signature = "public override void " + node.TargetMemberName + "(";
                bool first = true;
                for (const auto& pin : definition->OutputPins)
                {
                    if (pin.Type != "Exec")
                    {
                        if (!first) signature += ", ";
                        
                        std::string pinNameSanitized = pin.Name; 
                        signature += pin.Type + " " + pinNameSanitized;
                        first = false;
                        ctx.valueCache[{node.ID, pin.Name}] = pinNameSanitized;
                    }
                }
                signature += ")";
                ctx.AppendLine(signature);
                ctx.AppendLine("{");
                ctx.indent++;
                GenerateExecChain(ctx, node.ID, "然后");
                ctx.indent--;
                ctx.AppendLine("}");
                ctx.AppendLine("");
            }
        }

        
        for (const auto& func : blueprintData.Functions)
        {
            const BlueprintNode* entryNode = nullptr;
            for (const auto& node : blueprintData.Nodes)
            {
                if (node.Type == BlueprintNodeType::FunctionEntry && node.TargetMemberName == func.Name)
                {
                    entryNode = &node;
                    break;
                }
            }

            if (!entryNode) continue;

            std::string signature = func.Visibility + " " + func.ReturnType + " " + func.Name + "(";
            bool first = true;
            for (const auto& param : func.Parameters)
            {
                if (!first) signature += ", ";
                signature += param.Type + " " + param.Name;
                first = false;
                ctx.valueCache[{entryNode->ID, param.Name}] = param.Name;
            }
            signature += ")";
            ctx.AppendLine(signature);
            ctx.AppendLine("{");
            ctx.indent++;
            GenerateExecChain(ctx, entryNode->ID, "然后");
            ctx.indent--;
            ctx.AppendLine("}");
            ctx.AppendLine("");
        }

        ctx.indent--;
        ctx.AppendLine("}");
        ctx.indent--;
        ctx.AppendLine("}");

        return ctx.ss.str();
    }
}

const std::vector<std::string>& BlueprintImporter::GetSupportedExtensions() const
{
    static const std::vector<std::string> extensions = {".blueprint"};
    return extensions;
}

AssetMetadata BlueprintImporter::Import(const std::filesystem::path& assetPath)
{
    AssetMetadata metadata;
    metadata.guid = Guid::NewGuid();
    metadata.assetPath = Path::GetRelativePath(assetPath, AssetManager::GetInstance().GetAssetsRootPath());
    metadata.fileHash = Utils::GetHashFromFile(assetPath.string());
    metadata.type = AssetType::Blueprint;
    metadata.importerSettings = YAML::LoadFile(assetPath.string());

    try
    {
        Blueprint blueprintData = metadata.importerSettings.as<Blueprint>();
        std::string csharpCode = BlueprintCSharpCodeGenerator::Generate(blueprintData);

        std::filesystem::path scriptPath = blueprintData.Name;
        scriptPath.replace_extension(".g.cs");
        Path::WriteFile(scriptPath.string(), csharpCode);
        LogInfo("Generated C# script for blueprint '{}' at '{}'", metadata.assetPath.string(), scriptPath.string());
    }
    catch (const std::exception& e)
    {
        LogError("Failed to parse blueprint and generate C# script for '{}'. Error: {}", assetPath.string(), e.what());
    }

    return metadata;
}

AssetMetadata BlueprintImporter::Reimport(const AssetMetadata& metadata)
{
    AssetMetadata updatedMeta = metadata;
    std::filesystem::path fullPath = AssetManager::GetInstance().GetAssetsRootPath() / metadata.assetPath;

    updatedMeta.fileHash = Utils::GetHashFromFile(fullPath.string());
    updatedMeta.importerSettings = YAML::LoadFile(fullPath.string());

    try
    {
        Blueprint blueprintData = updatedMeta.importerSettings.as<Blueprint>();
        std::string csharpCode = BlueprintCSharpCodeGenerator::Generate(blueprintData);

        std::filesystem::path scriptPath = fullPath;
        scriptPath.replace_extension(".g.cs");
        Path::WriteFile(scriptPath.string(), csharpCode);
        LogInfo("Re-generated C# script for blueprint '{}'", metadata.assetPath.string());
    }
    catch (const std::exception& e)
    {
        LogError("Failed to re-import blueprint '{}'. Error: {}", fullPath.string(), e.what());
    }

    return updatedMeta;
}
