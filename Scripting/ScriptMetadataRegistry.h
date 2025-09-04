#ifndef SCRIPTMETADATAREGISTRY_H
#define SCRIPTMETADATAREGISTRY_H

#include "../Utils/LazySingleton.h"
#include "ScriptMetadata.h"
#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>

/**
 * @brief 脚本元数据注册表，用于管理和访问所有脚本元数据。
 *
 * 负责从YAML文件中加载脚本类元数据和所有可用类型列表。
 * 采用懒汉式单例模式。
 */
class ScriptMetadataRegistry : public LazySingleton<ScriptMetadataRegistry>
{
public:
    friend class LazySingleton<ScriptMetadataRegistry>;

    /**
     * @brief 初始化注册表，加载元数据。
     * @param metadataFilePath 元数据文件的路径。
     */
    void Initialize(const std::filesystem::path& metadataFilePath = "./ScriptMetadata.yaml");

    /**
     * @brief 刷新注册表中的元数据，重新加载文件。
     * @return 如果刷新成功则返回 `true`，否则返回 `false`。
     */
    bool Refresh();

    /**
     * @brief 获取指定类的元数据。
     * @param fullClassName 类的完整名称。
     * @return 对应类的元数据。如果不存在则返回无效的元数据对象。
     */
    ScriptClassMetadata GetMetadata(const std::string& fullClassName) const;

    /**
     * @brief 获取所有类的元数据。
     * @return 包含所有类元数据的常量映射引用。
     */
    const std::unordered_map<std::string, ScriptClassMetadata>& GetAllMetadata() const;

    /**
     * @brief 获取所有可用类型的列表。
     * @return 包含所有可用类型名称的常量向量引用。
     */
    const std::vector<std::string>& GetAvailableTypes() const;


private:
    ScriptMetadataRegistry() = default;

    std::filesystem::path m_metadataFilePath; ///< 元数据文件的路径。

    std::unordered_map<std::string, ScriptClassMetadata> m_classMetadata; ///< 存储所有脚本类元数据的映射。
    std::vector<std::string> m_availableTypes; ///< ✨ 新增: 存储所有可用类型的列表。
};

#endif //SCRIPTMETADATAREGISTRY_H