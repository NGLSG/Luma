#ifndef LUMAENGINE_RUNTIMESHADER_H
#define LUMAENGINE_RUNTIMESHADER_H
#include "IRuntimeAsset.h"
#include "ShaderData.h"
#include "include/effects/SkRuntimeEffect.h"
#include "Nut/Shader.h"


class RuntimeShader : public IRuntimeAsset
{
    std::string shaderCode;
    Nut::ShaderModule wgpuShader;
    Nut::ShaderModule computeShader;
    Data::ShaderLanguage language;
    Data::ShaderType shaderType;

public:
    RuntimeShader(const Data::ShaderData& shaderData, const std::shared_ptr<Nut::NutContext>& context,
                  const Guid& sourceGuid)
    {
        m_sourceGuid = sourceGuid;

        language = shaderData.language;
        shaderType = shaderData.type;
        shaderCode = shaderData.source;
        if (shaderData.language == Data::ShaderLanguage::WGSL)
        {
            if (!context)
            {
                LogError("RuntimeShader - NutContext is null for WGSL shader");
                return;
            }
            if (shaderData.type == Data::ShaderType::VertFrag)
            {
                wgpuShader = Nut::ShaderManager::GetFromString(shaderData.source, context);
            }
            else if (shaderData.type == Data::ShaderType::Compute)
            {
                computeShader = Nut::ShaderManager::GetFromString(shaderData.source, context);
            }
        }
    }

    ~RuntimeShader() override = default;

    [[nodiscard]] const std::string& GetSource() const { return shaderCode; }
    [[nodiscard]] const Nut::ShaderModule& GetWGPUShader() const { return wgpuShader; }
    [[nodiscard]] const Nut::ShaderModule& GetComputeShader() const { return computeShader; }
    [[nodiscard]] Data::ShaderLanguage GetLanguage() const { return language; }
    [[nodiscard]] Data::ShaderType GetShaderType() const { return shaderType; }
};


#endif //LUMAENGINE_RUNTIMESHADER_H
