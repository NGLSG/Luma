#include "SHA256.h"
#include "Logger.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <memory>

namespace Nut
{
    std::string SHA256::Hash(const std::string& data)
    {
        return Hash(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    }

    std::string SHA256::Hash(const uint8_t* data, size_t size)
    {
        auto rawHash = HashRaw(data, size);
        return BytesToHex(rawHash.data(), rawHash.size());
    }

    std::vector<uint8_t> SHA256::HashRaw(const std::string& data)
    {
        return HashRaw(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
    }

    std::vector<uint8_t> SHA256::HashRaw(const uint8_t* data, size_t size)
    {
        std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);

        
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if (!context)
        {
            LogError("SHA256::HashRaw - Failed to create EVP_MD_CTX");
            return hash;
        }

        if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1)
        {
            LogError("SHA256::HashRaw - Failed to initialize digest");
            EVP_MD_CTX_free(context);
            return hash;
        }

        if (EVP_DigestUpdate(context, data, size) != 1)
        {
            LogError("SHA256::HashRaw - Failed to update digest");
            EVP_MD_CTX_free(context);
            return hash;
        }

        unsigned int hashLength = 0;
        if (EVP_DigestFinal_ex(context, hash.data(), &hashLength) != 1)
        {
            LogError("SHA256::HashRaw - Failed to finalize digest");
            EVP_MD_CTX_free(context);
            return hash;
        }

        EVP_MD_CTX_free(context);

        if (hashLength != SHA256_DIGEST_LENGTH)
        {
            LogError("SHA256::HashRaw - Unexpected hash length: {}", hashLength);
        }

        return hash;
    }

    std::string SHA256::BytesToHex(const uint8_t* data, size_t size)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < size; ++i)
        {
            oss << std::setw(2) << static_cast<int>(data[i]);
        }
        return oss.str();
    }
}
