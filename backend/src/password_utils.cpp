/*
Модуль: password_utils.cpp
Назначение: Реализация утилит хеширования паролей и генерации ID
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Quality.Security.AccessAndStorage, LLR_RequirementService_Create_01
*/

#include "password_utils.hpp"

#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <sstream>

static const int SALT_LENGTH = 16;
static const int HASH_ITERATIONS = 100000;
static const int KEY_LENGTH = 32;

/*
Назначение: Преобразование байт в hex-строку
Входные данные: указатель на данные, длина
Выходные данные: hex-строка
*/
static std::string bytes_to_hex(const unsigned char* data, int len)
{
    std::ostringstream oss;
    for (int i = 0; i < len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
    }
    return oss.str();
}

/*
Назначение: Преобразование hex-строки в байты
Входные данные: hex-строка
Выходные данные: вектор байт
*/
static std::vector<unsigned char> hex_to_bytes(const std::string& hex)
{
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned int byte = 0;
        std::istringstream(hex.substr(i, 2)) >> std::hex >> byte;
        bytes.push_back(static_cast<unsigned char>(byte));
    }
    return bytes;
}

std::string hash_password(const std::string& password)
{
    unsigned char salt[SALT_LENGTH];
    RAND_bytes(salt, SALT_LENGTH);

    unsigned char key[KEY_LENGTH];
    PKCS5_PBKDF2_HMAC(
        password.c_str(), static_cast<int>(password.size()),
        salt, SALT_LENGTH, HASH_ITERATIONS,
        EVP_sha256(), KEY_LENGTH, key
    );

    std::string salt_hex = bytes_to_hex(salt, SALT_LENGTH);
    std::string key_hex = bytes_to_hex(key, KEY_LENGTH);
    return salt_hex + ":" + key_hex;
}

bool check_password(const std::string& password, const std::string& hash)
{
    auto sep = hash.find(':');
    if (sep == std::string::npos) return false;

    std::string salt_hex = hash.substr(0, sep);
    std::string stored_key_hex = hash.substr(sep + 1);
    auto salt_bytes = hex_to_bytes(salt_hex);

    unsigned char key[KEY_LENGTH];
    PKCS5_PBKDF2_HMAC(
        password.c_str(), static_cast<int>(password.size()),
        salt_bytes.data(), static_cast<int>(salt_bytes.size()),
        HASH_ITERATIONS, EVP_sha256(), KEY_LENGTH, key
    );

    std::string computed_hex = bytes_to_hex(key, KEY_LENGTH);
    return computed_hex == stored_key_hex;
}

std::string generate_req_id()
{
    unsigned char buf[4];
    RAND_bytes(buf, 4);
    std::string hex = bytes_to_hex(buf, 4);
    for (auto& ch : hex) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return "REQ-" + hex;
}

std::string generate_session_token()
{
    unsigned char buf[32];
    RAND_bytes(buf, 32);
    return bytes_to_hex(buf, 32);
}
