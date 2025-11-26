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
    [[nodiscard]] const std::string& GetShaderCode() const { return shaderCode; }
    [[nodiscard]] const Nut::ShaderModule& GetWGPUShader() const { return wgpuShader; }
    [[nodiscard]] const Nut::ShaderModule& GetComputeShader() const { return computeShader; }
    [[nodiscard]] Data::ShaderLanguage GetLanguage() const { return language; }
    [[nodiscard]] Data::ShaderType GetShaderType() const { return shaderType; }

    /**
     * @brief 确保 shader 已编译到 GPU
     * 这会触发 Dawn Blob Cache 的保存
     */
    void EnsureCompiled()
    {
        // ShaderModule 在构造时已经编译
        // 这里只是确保访问，触发缓存
        if (shaderType == Data::ShaderType::VertFrag && wgpuShader.Get())
        {
            // 访问 shader module 确保已编译
            [[maybe_unused]] auto module = wgpuShader.Get();
        }
        else if (shaderType == Data::ShaderType::Compute && computeShader.Get())
        {
            [[maybe_unused]] auto module = computeShader.Get();
        }
    }
};


#endif //LUMAENGINE_RUNTIMESHADER_H
