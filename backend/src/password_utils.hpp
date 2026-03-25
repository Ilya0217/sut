/*
Модуль: password_utils.hpp
Назначение: Утилиты хеширования паролей
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Quality.Security.AccessAndStorage
*/

#ifndef PASSWORD_UTILS_HPP
#define PASSWORD_UTILS_HPP

#include <string>

std::string hash_password(const std::string& password);
bool check_password(const std::string& password, const std::string& hash);
std::string generate_req_id();
std::string generate_session_token();

#endif
