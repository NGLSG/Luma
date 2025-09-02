#pragma once
#include <string>
#include <array>
#include <cstdint>
#include <compare>

#include "yaml-cpp/yaml.h"
#include "Platform.h"

/**
 * @brief Guid 类，表示一个 16 字节的全局唯一标识符（UUID/GUID）。
 */
class LUMA_API Guid
{
private:
    std::array<uint8_t, 16> m_data; ///< 存储 GUID 的 16 字节数据

public:
    /**
     * @brief 默认构造函数，生成无效的 Guid。
     */
    Guid();

    /**
     * @brief 通过字节数组构造 Guid。
     * @param bytes 16 字节数组
     */
    explicit Guid(const std::array<uint8_t, 16>& bytes);

    /**
     * @brief 通过字符串构造 Guid。
     * @param str GUID 字符串
     */
    Guid(const std::string& str);

    /**
     * @brief 生成一个新的随机 Guid。
     * @return 新的 Guid 对象
     */
    static Guid NewGuid();

    /**
     * @brief 通过字符串解析生成 Guid。
     * @param str GUID 字符串
     * @return 解析得到的 Guid 对象
     */
    static Guid FromString(const std::string& str);

    /**
     * @brief 将 Guid 转换为字符串表示。
     * @return GUID 字符串
     */
    std::string ToString() const;

    /**
     * @brief 获取 GUID 的 C 风格字符串指针。
     * @return C 字符串指针
     */
    const char* c_str() const;

    /**
     * @brief 判断 Guid 是否有效。
     * @return 有效返回 true，否则返回 false
     */
    bool Valid() const;

    /**
     * @brief 获取 Guid 的字节数组。
     * @return 16 字节数组的常量引用
     */
    const std::array<uint8_t, 16>& GetBytes() const;

    /**
     * @brief 判断两个 Guid 是否相等。
     * @param other 另一个 Guid
     * @return 相等返回 true，否则返回 false
     */
    bool operator==(const Guid& other) const;

    /**
     * @brief 判断两个 Guid 是否不相等。
     * @param other 另一个 Guid
     * @return 不相等返回 true，否则返回 false
     */
    bool operator!=(const Guid& other) const;

    /**
     * @brief Guid 隐式转换为字符串。
     * @return GUID 字符串
     */
    operator std::string() const;

    /**
     * @brief Guid 隐式转换为字节数组。
     * @return 16 字节数组
     */
    operator std::array<uint8_t, 16>() const;

    /**
     * @brief 获取一个无效的 Guid 单例。
     * @return 无效 Guid 的引用
     */
    static Guid& Invalid();
};

/**
 * @brief YAML 序列化与反序列化 Guid 的特化模板。
 */
namespace YAML
{
    template <>
    struct convert<Guid>
    {
        /**
         * @brief 将 Guid 编码为 YAML 节点。
         * @param guid Guid 对象
         * @return YAML 节点
         */
        static Node encode(const Guid& guid)
        {
            return Node(guid.ToString());
        }

        /**
         * @brief 从 YAML 节点解码为 Guid。
         * @param node YAML 节点
         * @param guid Guid 对象引用
         * @return 解码成功返回 true，否则返回 false
         */
        static bool decode(const Node& node, Guid& guid)
        {
            if (!node.IsDefined() || node.IsNull())
            {
                guid = Guid();
                return true;
            }

            if (!node.IsScalar())
            {
                return false;
            }

            std::string guidStr = node.as<std::string>();

            if (guidStr.empty())
            {
                guid = Guid();
                return true;
            }

            try
            {
                guid = Guid::FromString(guidStr);

                return true;
            }
            catch (const std::invalid_argument& e)
            {
                return false;
            }
        }
    };
}

/**
 * @brief std::hash 对 Guid 的特化，用于哈希容器。
 */
namespace std
{
    template <>
    struct hash<Guid>
    {
        /**
         * @brief 计算 Guid 的哈希值。
         * @param guid Guid 对象
         * @return 哈希值
         */
        size_t operator()(const Guid& guid) const noexcept
        {
            const auto& bytes = guid.GetBytes();
            const uint64_t* p = reinterpret_cast<const uint64_t*>(bytes.data());
            return hash<uint64_t>{}(p[0]) ^ (hash<uint64_t>{}(p[1]) << 1);
        }
    };
}