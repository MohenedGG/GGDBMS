#include "SnapshotEncryptor.h"
#include <cctype>

namespace
{
    constexpr const char *kEncryptedSnapshotMagic = "GGDBMS_SNAPSHOT_ENC_V1";
    constexpr const char *kSimpleXorKey = "GGDB_SIMPLE_KEY_V1";
}

const std::string &SnapshotEncryptor::magicHeader()
{
    static const std::string header = kEncryptedSnapshotMagic;
    return header;
}

std::string SnapshotEncryptor::encryptToHex(const std::string &plainText)
{
    return toHex(xorTransform(plainText));
}

bool SnapshotEncryptor::decryptFromHex(const std::string &hexText, std::string &plainText)
{
    std::string encrypted;
    if (!fromHex(hexText, encrypted))
    {
        return false;
    }

    plainText = xorTransform(encrypted);
    return true;
}

std::string SnapshotEncryptor::xorTransform(const std::string &input)
{
    std::string output = input;
    const size_t keyLength = std::char_traits<char>::length(kSimpleXorKey);

    if (keyLength == 0)
    {
        return output;
    }

    for (size_t i = 0; i < output.size(); ++i)
    {
        output[i] = static_cast<char>(output[i] ^ kSimpleXorKey[i % keyLength]);
    }

    return output;
}

std::string SnapshotEncryptor::toHex(const std::string &binary)
{
    static constexpr char kHexDigits[] = "0123456789ABCDEF";

    std::string hex;
    hex.reserve(binary.size() * 2);

    for (unsigned char byte : binary)
    {
        hex.push_back(kHexDigits[(byte >> 4) & 0x0F]);
        hex.push_back(kHexDigits[byte & 0x0F]);
    }

    return hex;
}

bool SnapshotEncryptor::fromHex(const std::string &hex, std::string &binary)
{
    if (hex.size() % 2 != 0)
    {
        return false;
    }

    binary.clear();
    binary.reserve(hex.size() / 2);

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        unsigned char high = 0;
        unsigned char low = 0;

        if (!hexNibble(hex[i], high) || !hexNibble(hex[i + 1], low))
        {
            return false;
        }

        binary.push_back(static_cast<char>((high << 4) | low));
    }

    return true;
}

bool SnapshotEncryptor::hexNibble(char ch, unsigned char &nibble)
{
    if (ch >= '0' && ch <= '9')
    {
        nibble = static_cast<unsigned char>(ch - '0');
        return true;
    }

    const unsigned char upper = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(ch)));
    if (upper >= 'A' && upper <= 'F')
    {
        nibble = static_cast<unsigned char>(10 + (upper - 'A'));
        return true;
    }

    return false;
}
