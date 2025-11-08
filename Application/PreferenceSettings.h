#ifndef PREFERENCESETTINGS_H
#define PREFERENCESETTINGS_H

#include "../Utils/LazySingleton.h"
#include "../Utils/IDEIntegration.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>

/**
 * @brief 偏好设置类，用于管理和存储用户偏好设置。
 *
 * 该类继承自 LazySingleton，确保在整个应用程序中只有一个实例。
 * 它负责加载、保存和提供对各种用户偏好设置的访问，例如首选的 IDE。
 */
class PreferenceSettings : public LazySingleton<PreferenceSettings>
{
public:
    friend class LazySingleton<PreferenceSettings>;

    /**
     * @brief 初始化偏好设置，加载指定路径的配置文件。
     *
     * 如果配置文件不存在，可能会创建默认设置。
     *
     * @param configPath 配置文件的路径。
     */
    void Initialize(const std::filesystem::path& configPath);

    /**
     * @brief 保存当前的偏好设置到配置文件。
     *
     * 将所有修改过的偏好设置持久化到磁盘上的配置文件中。
     */
    void Save();

    /**
     * @brief 获取用户首选的集成开发环境 (IDE)。
     *
     * @return 当前设置的首选 IDE。
     */
    IDE GetPreferredIDE() const;

    /**
     * @brief 设置用户首选的集成开发环境 (IDE)。
     *
     * @param ide 要设置的首选 IDE。
     */
    void SetPreferredIDE(IDE ide);

    const std::filesystem::path& GetAndroidSdkPath() const { return m_androidSdkPath; }
    const std::filesystem::path& GetAndroidNdkPath() const { return m_androidNdkPath; }
    void SetAndroidSdkPath(const std::filesystem::path& path);
    void SetAndroidNdkPath(const std::filesystem::path& path);
    std::filesystem::path GetLibcxxSharedPath(const std::string& abi) const;

    /**
     * @brief 默认构造函数。
     */
    PreferenceSettings() = default;

private:
    /**
     * @brief 从配置文件加载偏好设置。
     *
     * 这是一个私有方法，由 Initialize 调用。
     */
    void Load();

    /// 用户首选的集成开发环境 (IDE)。
    IDE m_preferredIDE = IDE::Unknown;
    std::filesystem::path m_androidSdkPath;
    std::filesystem::path m_androidNdkPath;

    /// 配置文件的路径。
    std::filesystem::path m_configPath;
};

namespace YAML
{
    /**
     * @brief YAML 转换器特化，用于 PreferenceSettings 对象的序列化和反序列化。
     */
    template <>
    struct convert<PreferenceSettings>
    {
        /**
         * @brief 将 PreferenceSettings 对象编码为 YAML 节点。
         *
         * @param rhs 要编码的 PreferenceSettings 对象。
         * @return 表示 PreferenceSettings 对象的 YAML 节点。
         */
        static Node encode(const PreferenceSettings& rhs);

        /**
         * @brief 从 YAML 节点解码 PreferenceSettings 对象。
         *
         * @param node 包含 PreferenceSettings 数据的 YAML 节点。
         * @param rhs 用于存储解码结果的 PreferenceSettings 对象。
         * @return 如果解码成功则返回 true，否则返回 false。
         */
        static bool decode(const Node& node, PreferenceSettings& rhs);
    };
}

#endif
