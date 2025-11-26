#ifndef LUMAENGINE_SHADERLOADER_H
#define LUMAENGINE_SHADERLOADER_H

#include "IAssetLoader.h"
#include "RuntimeAsset/RuntimeShader.h"


class ShaderLoader : public IAssetLoader<RuntimeShader>
{
private:
    std::shared_ptr<Nut::NutContext> context;

public:
    explicit ShaderLoader(const std::shared_ptr<Nut::NutContext>& context) : context(context)
    {
    }

    sk_sp<RuntimeShader> LoadAsset(const AssetMetadata& metadata) override;
    sk_sp<RuntimeShader> LoadAsset(const Guid& Guid) override;
    Data::ShaderData LoadShaderDataFromGuid(const Guid& guid);
};



#endif //LUMAENGINE_SHADERLOADER_H
