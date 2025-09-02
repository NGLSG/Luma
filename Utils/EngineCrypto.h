#ifndef ENGINECYRPTO_H
#define ENGINECYRPTO_H

#include "LazySingleton.h"
#include <vector>
#include <string>

/**
 * @brief EngineCrypto 类，提供加解密相关功能，采用单例模式。
 *
 * 该类用于对数据进行加密和解密操作，内部包含密钥派生、数据加解密、MAC 计算与校验、随机字节生成等功能。
 */
class EngineCrypto : public LazySingleton<EngineCrypto>
{
public:
    friend class LazySingleton<EngineCrypto>;

    /**
     * @brief 加密数据。
     * @param data 待加密的原始数据
     * @return 加密后的数据包
     */
    std::vector<unsigned char> Encrypt(const std::vector<unsigned char>& data);

    /**
     * @brief 解密数据包。
     * @param encryptedPackage 加密后的数据包
     * @return 解密后的原始数据
     */
    std::vector<unsigned char> Decrypt(const std::vector<unsigned char>& encryptedPackage);

private:
    /**
     * @brief 构造函数，私有以实现单例模式。
     */
    EngineCrypto() = default;

    /**
     * @brief 析构函数，私有以实现单例模式。
     */
    ~EngineCrypto() = default;

    /**
     * @brief 派生密钥。
     * @param salt 盐值
     * @param keyLength 密钥长度
     * @param purpose 用途标识（可选）
     * @return 派生出的密钥
     */
    std::vector<unsigned char> deriveKey(const std::vector<unsigned char>& salt, int keyLength,
                                         const std::string& purpose = "");

    /**
     * @brief 加密数据（内部实现）。
     * @param data 待加密数据
     * @param key 加密密钥
     * @param iv 初始向量
     * @return 加密后的数据
     */
    std::vector<unsigned char> encryptData(const std::vector<unsigned char>& data,
                                           const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);

    /**
     * @brief 解密数据（内部实现）。
     * @param encryptedData 加密数据
     * @param key 解密密钥
     * @param iv 初始向量
     * @return 解密后的数据
     */
    std::vector<unsigned char> decryptData(const std::vector<unsigned char>& encryptedData,
                                           const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);

    /**
     * @brief 计算消息认证码（MAC）。
     * @param data 输入数据
     * @param key MAC 密钥
     * @return 计算得到的 MAC
     */
    std::vector<unsigned char> computeMac(const std::vector<unsigned char>& data,
                                          const std::vector<unsigned char>& key);

    /**
     * @brief 比较两个 MAC 是否一致。
     * @param mac1 第一个 MAC
     * @param mac2 第二个 MAC
     * @return 一致返回 true，否则返回 false
     */
    bool compareMacs(const std::vector<unsigned char>& mac1, const std::vector<unsigned char>& mac2);

    /**
     * @brief 生成指定长度的随机字节。
     * @param length 字节长度
     * @return 随机字节数组
     */
    std::vector<unsigned char> generateRandomBytes(int length);
};

#endif