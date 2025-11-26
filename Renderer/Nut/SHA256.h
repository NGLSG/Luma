#ifndef SHA256_H
#define SHA256_H

#include <string>
#include <vector>
#include <cstdint>

namespace Nut
{
    /**
     * @brief SHA256 哈希工具类（使用 OpenSSL 3+）
     */
    class SHA256
    {
    public:
        /**
         * @brief 计算字符串的 SHA256 哈希
         * @param data 输入数据
         * @return 32 字节的哈希值（十六进制字符串）
         */
        static std::string Hash(const std::string& data);

        /**
         * @brief 计算二进制数据的 SHA256 哈希
         * @param data 输入数据指针
         * @param size 数据大小
         * @return 32 字节的哈希值（十六进制字符串）
         */
        static std::string Hash(const uint8_t* data, size_t size);

        /**
         * @brief 计算字符串的 SHA256 哈希（返回原始字节）
         * @param data 输入数据
         * @return 32 字节的原始哈希值
         */
        static std::vector<uint8_t> HashRaw(const std::string& data);

        /**
         * @brief 计算二进制数据的 SHA256 哈希（返回原始字节）
         * @param data 输入数据指针
         * @param size 数据大小
         * @return 32 字节的原始哈希值
         */
        static std::vector<uint8_t> HashRaw(const uint8_t* data, size_t size);

    private:
        /**
         * @brief 将字节数组转换为十六进制字符串
         */
        static std::string BytesToHex(const uint8_t* data, size_t size);
    };
}

#endif // SHA256_H
