#include "Guid.h"
#include "Guid.h"
#include <stdexcept>
#include <format>
#include <openssl/rand.h>

Guid::Guid() : m_data{}
{
}

Guid::Guid(const std::array<uint8_t, 16>& bytes) : m_data(bytes)
{
}

Guid::Guid(const std::string& str)
{
    *this = FromString(str);
}

Guid Guid::NewGuid()
{
    std::array<uint8_t, 16> bytes;


    if (RAND_bytes(bytes.data(), bytes.size()) != 1)
    {
        throw std::runtime_error("Failed to generate random bytes for GUID using OpenSSL.");
    }


    bytes[6] = (bytes[6] & 0x0F) | 0x40;


    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    return Guid(bytes);
}


Guid Guid::FromString(const std::string& str)
{
    static std::unordered_map<std::string, Guid> s_guidCache;
    static std::mutex s_cacheMutex;


    {
        std::lock_guard<std::mutex> lock(s_cacheMutex);
        auto it = s_guidCache.find(str);
        if (it != s_guidCache.end())
        {
            return it->second;
        }
    }


    if (str.length() != 36)
    {
        throw std::invalid_argument("Invalid GUID string length.");
    }

    if (str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-')
    {
        throw std::invalid_argument("Invalid GUID string format: hyphens are misplaced.");
    }

    std::array<uint8_t, 16> bytes{};
    const char* ptr = str.c_str();
    size_t byteIndex = 0;

    auto parse_hex_block = [&](size_t byte_count)
    {
        for (size_t i = 0; i < byte_count; ++i)
        {
            auto result = std::from_chars(ptr, ptr + 2, bytes[byteIndex++], 16);
            if (result.ec != std::errc())
            {
                throw std::invalid_argument("Invalid hexadecimal character in GUID string.");
            }
            ptr += 2;
        }
    };

    try
    {
        parse_hex_block(4);
        ptr++;
        parse_hex_block(2);
        ptr++;
        parse_hex_block(2);
        ptr++;
        parse_hex_block(2);
        ptr++;
        parse_hex_block(6);
    }
    catch (const std::out_of_range&)
    {
        throw std::invalid_argument("GUID string is malformed.");
    }

    Guid newGuid(bytes);


    {
        std::lock_guard<std::mutex> lock(s_cacheMutex);
        s_guidCache[str] = newGuid;
    }

    return newGuid;
}

std::string Guid::ToString() const
{
    return std::format("{:02x}{:02x}{:02x}{:02x}-"
                       "{:02x}{:02x}-"
                       "{:02x}{:02x}-"
                       "{:02x}{:02x}-"
                       "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                       m_data[0], m_data[1], m_data[2], m_data[3],
                       m_data[4], m_data[5],
                       m_data[6], m_data[7],
                       m_data[8], m_data[9],
                       m_data[10], m_data[11], m_data[12], m_data[13], m_data[14], m_data[15]);
}

const char* Guid::c_str() const
{
    thread_local std::string buffer;
    buffer = ToString();
    return buffer.c_str();
}

bool Guid::Valid() const
{
    return m_data != std::array<uint8_t, 16>{} || this == &Invalid();
}

bool Guid::operator==(const Guid& other) const
{
    return m_data == other.m_data;
}

bool Guid::operator!=(const Guid& other) const
{
    return !(*this == other);
}

const std::array<uint8_t, 16>& Guid::GetBytes() const
{
    return m_data;
}

Guid::operator std::string() const
{
    return ToString();
}

Guid::operator std::array<unsigned char, 16>() const
{
    return m_data;
}

Guid& Guid::Invalid()
{
    static Guid invalidGuid;
    return invalidGuid;
}
