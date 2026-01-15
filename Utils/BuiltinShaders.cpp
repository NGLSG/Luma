#include "BuiltinShaders.h"
#include "../Renderer/Nut/ShaderModuleRegistry.h"
#include "../Renderer/Nut/ShaderModuleInitializer.h"
#include <fstream>
#include <sstream>

/**
 * 内建Shader列表
 * 只包含完整的shader（没有export语句，可直接用于材质渲染）
 * 使用固定GUID格式: 00000000-0000-0000-0000-00000000000X
 * 
 * 注意：有export语句的文件是模块，供用户import使用，不在此列表中
 */
const std::vector<BuiltinShaderInfo> BuiltinShaders::s_builtinShaders = {
    {"SpriteLit (内建)", "00000000-0000-0000-0000-000000000001", "SpriteLit.wgsl"},
    {"Particle (内建)", "00000000-0000-0000-0000-000000000002", "Particle.wgsl"},
    {"Shadow (内建)", "00000000-0000-0000-0000-000000000003", "Shadow.wgsl"},
};

const std::vector<BuiltinShaderInfo>& BuiltinShaders::GetAllBuiltinShaders()
{
    return s_builtinShaders;
}

bool BuiltinShaders::IsBuiltinShaderGuid(const Guid& guid)
{
    std::string guidStr = guid.ToString();
    for (const auto& builtin : s_builtinShaders)
    {
        if (builtin.guidStr == guidStr) return true;
    }
    return false;
}

std::string BuiltinShaders::GetBuiltinShaderName(const Guid& guid)
{
    std::string guidStr = guid.ToString();
    for (const auto& builtin : s_builtinShaders)
    {
        if (builtin.guidStr == guidStr) return builtin.name;
    }
    return "";
}

std::string BuiltinShaders::GetBuiltinShaderModuleName(const Guid& guid)
{
    std::string guidStr = guid.ToString();
    for (const auto& builtin : s_builtinShaders)
    {
        if (builtin.guidStr == guidStr) return builtin.moduleName;
    }
    return "";
}

Data::ShaderData BuiltinShaders::GetBuiltinShaderData(const Guid& guid)
{
    Data::ShaderData shaderData;
    
    std::string fileName = GetBuiltinShaderModuleName(guid);
    if (fileName.empty())
    {
        return shaderData;
    }

    // 从Shaders目录读取完整shader文件
    auto shadersPath = Nut::ShaderModuleInitializer::GetDefaultShadersPath();
    auto filePath = shadersPath / fileName;
    
    std::ifstream file(filePath);
    if (file.is_open())
    {
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        shaderData.type = Data::ShaderType::VertFrag;
        shaderData.language = Data::ShaderLanguage::WGSL;
        shaderData.source = buffer.str();
    }

    return shaderData;
}
