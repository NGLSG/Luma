#include "EngineCrypto.h"
#include <stdexcept>
#include <vector>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>


constexpr int SALT_SIZE = 16;
constexpr int IV_SIZE = 16;
constexpr int KEY_SIZE = 32;
constexpr int MAC_SIZE = 32;
constexpr int ITERATIONS = 10000;


const std::vector<unsigned char> KEY_COMPONENT_1 = {
    'L', 'u', 'm', 'a', 'E', 'n', 'g', 'i', 'n', 'e', 'P', 'a', 'c', 'k', 'a', 'g', 'e'
};
const std::vector<unsigned char> KEY_COMPONENT_2 = {
    'S', 'e', 'c', 'r', 'e', 't', 'A', 's', 's', 'e', 't', 'K', 'e', 'y', '2', '0', '2', '5'
};
const std::vector<unsigned char> KEY_COMPONENT_3 = {
    0x2A, 0x7B, 0x8C, 0x9D, 0xAE, 0xBF, 0xC0, 0xD1
};

std::vector<unsigned char> EngineCrypto::Encrypt(const std::vector<unsigned char>& data)
{
    auto salt = generateRandomBytes(SALT_SIZE);
    auto iv = generateRandomBytes(IV_SIZE);
    auto encryptionKey = deriveKey(salt, KEY_SIZE, "ENCRYPTION");
    auto macKey = deriveKey(salt, MAC_SIZE, "MAC");
    auto encryptedData = encryptData(data, encryptionKey, iv);

    std::vector<unsigned char> package;

    package.insert(package.end(), salt.begin(), salt.end());
    package.insert(package.end(), iv.begin(), iv.end());
    package.insert(package.end(), encryptedData.begin(), encryptedData.end());

    auto mac = computeMac(package, macKey);
    package.insert(package.end(), mac.begin(), mac.end());

    return package;
}

std::vector<unsigned char> EngineCrypto::Decrypt(const std::vector<unsigned char>& encryptedPackage)
{
    if (encryptedPackage.size() < (SALT_SIZE + IV_SIZE + MAC_SIZE))
    {
        throw std::runtime_error("Encrypted package is too small.");
    }
    size_t pos = 0;
    std::vector<unsigned char> salt(encryptedPackage.begin(), encryptedPackage.begin() + SALT_SIZE);
    pos += SALT_SIZE;
    std::vector<unsigned char> iv(encryptedPackage.begin() + pos, encryptedPackage.begin() + pos + IV_SIZE);
    pos += IV_SIZE;
    size_t encryptedDataSize = encryptedPackage.size() - SALT_SIZE - IV_SIZE - MAC_SIZE;
    std::vector<unsigned char> encryptedData(encryptedPackage.begin() + pos,
                                             encryptedPackage.begin() + pos + encryptedDataSize);
    pos += encryptedDataSize;
    std::vector<unsigned char> storedMac(encryptedPackage.begin() + pos, encryptedPackage.end());

    auto macKey = deriveKey(salt, MAC_SIZE, "MAC");
    std::vector<unsigned char> dataToVerify(encryptedPackage.begin(), encryptedPackage.begin() + pos);
    auto computedMac = computeMac(dataToVerify, macKey);
    if (!compareMacs(storedMac, computedMac))
    {
        throw std::runtime_error("Data integrity check failed (MAC mismatch).");
    }

    auto encryptionKey = deriveKey(salt, KEY_SIZE, "ENCRYPTION");
    return decryptData(encryptedData, encryptionKey, iv);
}


std::vector<unsigned char> EngineCrypto::generateRandomBytes(int length)

{
    std::vector<unsigned char> bytes(length);

    if (RAND_bytes(bytes.data(), length) != 1)

    {
        throw std::runtime_error("生成随机字节失败。");
    }

    return bytes;
}


std::vector<unsigned char> EngineCrypto::deriveKey(const std::vector<unsigned char>& salt, int keyLength,
                                                   const std::string& purpose)
{
    std::vector<unsigned char> keyMaterial;
    keyMaterial.insert(keyMaterial.end(), KEY_COMPONENT_1.begin(), KEY_COMPONENT_1.end());
    keyMaterial.insert(keyMaterial.end(), KEY_COMPONENT_2.begin(), KEY_COMPONENT_2.end());
    keyMaterial.insert(keyMaterial.end(), KEY_COMPONENT_3.begin(), KEY_COMPONENT_3.end());
    keyMaterial.insert(keyMaterial.end(), purpose.begin(), purpose.end());
    keyMaterial.insert(keyMaterial.end(), salt.begin(), salt.end());
    std::vector<unsigned char> derivedKey(keyLength);
    if (PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(keyMaterial.data()),
                          static_cast<int>(keyMaterial.size()),
                          salt.data(),
                          static_cast<int>(salt.size()),
                          ITERATIONS,
                          EVP_sha256(),
                          keyLength,
                          derivedKey.data()) != 1)
    {
        throw std::runtime_error("密钥派生失败。");
    }

    return derivedKey;
}


std::vector<unsigned char> EngineCrypto::encryptData(const std::vector<unsigned char>& data,
                                                     const std::vector<unsigned char>& key,
                                                     const std::vector<unsigned char>& iv)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        throw std::runtime_error("创建加密上下文失败。");
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("初始化加密失败。");
    }
    std::vector<unsigned char> encryptedData(data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int len = 0;
    int encryptedLen = 0;
    if (EVP_EncryptUpdate(ctx, encryptedData.data(), &len, data.data(), static_cast<int>(data.size())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("加密数据失败。");
    }
    encryptedLen = len;
    if (EVP_EncryptFinal_ex(ctx, encryptedData.data() + len, &len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("完成加密失败。");
    }
    encryptedLen += len;
    EVP_CIPHER_CTX_free(ctx);
    encryptedData.resize(encryptedLen);
    return encryptedData;
}


std::vector<unsigned char> EngineCrypto::decryptData(const std::vector<unsigned char>& encryptedData,
                                                     const std::vector<unsigned char>& key,
                                                     const std::vector<unsigned char>& iv)

{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        throw std::runtime_error("创建解密上下文失败。");
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("初始化解密失败。");
    }
    std::vector<unsigned char> decryptedData(encryptedData.size());
    int len = 0;
    int decryptedLen = 0;
    if (EVP_DecryptUpdate(ctx, decryptedData.data(), &len, encryptedData.data(),
                          static_cast<int>(encryptedData.size())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("解密数据失败。");
    }
    decryptedLen = len;
    if (EVP_DecryptFinal_ex(ctx, decryptedData.data() + len, &len) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("完成解密失败。数据可能已损坏或密钥/IV错误。");
    }
    decryptedLen += len;
    EVP_CIPHER_CTX_free(ctx);
    decryptedData.resize(decryptedLen);
    return decryptedData;
}


std::vector<unsigned char> EngineCrypto::computeMac(const std::vector<unsigned char>& data,
                                                    const std::vector<unsigned char>& key)
{
    unsigned int macLen;
    std::vector<unsigned char> mac(EVP_MD_size(EVP_sha256()));
    if (!HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()), data.data(), data.size(), mac.data(), &macLen))
    {
        throw std::runtime_error("计算MAC失败。");
    }
    mac.resize(macLen);
    return mac;
}


bool EngineCrypto::compareMacs(const std::vector<unsigned char>& mac1, const std::vector<unsigned char>& mac2)
{
    if (mac1.size() != mac2.size())
        return false;
    int result = 0;
    for (size_t i = 0; i < mac1.size(); i++)
    {
        result |= mac1[i] ^ mac2[i];
    }
    return result == 0;
}
