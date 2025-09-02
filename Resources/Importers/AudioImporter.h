/**
 * @brief 音频资源导入器。
 *
 * 负责导入和重新导入音频文件资源，并提供支持的扩展名列表。
 */
#ifndef AUDIOIMPORTER_H
#define AUDIOIMPORTER_H

#include "IAssetImporter.h"


class AudioImporter : public IAssetImporter
{
public:
    /**
     * @brief 获取该导入器支持的音频文件扩展名列表。
     * @return 支持的扩展名字符串向量的常量引用。
     */
    const std::vector<std::string>& GetSupportedExtensions() const override;

    /**
     * @brief 导入指定路径的音频资源。
     * @param assetPath 待导入音频资源的路径。
     * @return 导入后的资源元数据。
     */
    AssetMetadata Import(const std::filesystem::path& assetPath) override;

    /**
     * @brief 重新导入已存在的音频资源。
     * @param metadata 待重新导入资源的现有元数据。
     * @return 重新导入后的资源元数据。
     */
    AssetMetadata Reimport(const AssetMetadata& metadata) override;

private:
    void ReadFileBytes(const std::filesystem::path& assetPath, YAML::Node& settingsNode);
};

#endif