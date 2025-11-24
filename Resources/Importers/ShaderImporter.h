
#ifndef LUMAENGINE_SHADERLOADER_H
#define LUMAENGINE_SHADERLOADER_H
#include "IAssetImporter.h"


class ShaderImporter : public IAssetImporter
{
public:
    const std::vector<std::string>& GetSupportedExtensions() const override;
    AssetMetadata Import(const std::filesystem::path& assetPath) override;
    AssetMetadata Reimport(const AssetMetadata& metadata) override;
};



#endif //LUMAENGINE_SHADERLOADER_H
