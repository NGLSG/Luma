#ifndef NOAI_SHADER_H
#define NOAI_SHADER_H
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "dawn/webgpu_cpp.h"

class NutContext;

enum class BindingType:uint32_t
{
    UniformBuffer = 0,
    StorageBuffer = 1,
    Texture = 2,
    Sampler = 3,
};

struct ShaderBindingInfo
{
    size_t groupIndex;
    size_t location;
    BindingType type;
    std::string name;
};

class ShaderModule
{
    wgpu::ShaderModule shaderModule;
    std::unordered_map<std::string, ShaderBindingInfo> bindings;

public:
    ShaderModule(const std::string& shaderCode, const std::shared_ptr<NutContext>&ctx);

    ShaderModule();

    wgpu::ShaderModule& Get();
    operator bool() const;

    ShaderBindingInfo GetBindingInfo(const std::string& name);
    bool GetBindingInfo(const std::string& name, ShaderBindingInfo& info);

    void ForeachBinding(const std::function<void(const ShaderBindingInfo&)>& callback) const;
};

class ShaderManager
{
    inline static std::unordered_map<std::string, std::shared_ptr<ShaderModule>> shaderModules;

public:
    static ShaderModule& GetFromFile(const std::string& file, const std::shared_ptr<NutContext>& ctx);

    static ShaderModule& GetFromString(const std::string& code, const std::shared_ptr<NutContext>&ctx);
};
#endif //NOAI_SHADER_H
