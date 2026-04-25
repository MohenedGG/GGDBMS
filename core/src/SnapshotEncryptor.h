#pragma once
#include <string>

class SnapshotEncryptor
{
private:
    static std::string xorTransform(const std::string &input);
    static std::string toHex(const std::string &binary);
    static bool fromHex(const std::string &hex, std::string &binary);
    static bool hexNibble(char ch, unsigned char &nibble);

public:
    static const std::string &magicHeader();
    static std::string encryptToHex(const std::string &plainText);
    static bool decryptFromHex(const std::string &hexText, std::string &plainText);
};
