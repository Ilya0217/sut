/*
Модуль: password_utils.hpp
Назначение: Интерфейс утилит хеширования паролей и генерации ID
Автор: Разработчик
Дата создания: 25.03.2026
Требования: Quality.Security.AccessAndStorage
*/

#ifndef PASSWORD_UTILS_HPP
#define PASSWORD_UTILS_HPP

#include <string>
#include <vector>

std::string hash_password(const std::string& password);
bool check_password(const std::string& password, const std::string& hash);
std::string generate_req_id();
std::string generate_session_token();

#endif
