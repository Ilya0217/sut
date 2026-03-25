/*
Модуль: user_service.hpp
Назначение: Интерфейс сервиса управления пользователями
Автор: Разработчик
Дата создания: 25.03.2026
Требования: Functional.Security.UserManagement
*/

#ifndef USER_SERVICE_HPP
#define USER_SERVICE_HPP

#include "models.hpp"
#include <optional>
#include <stdexcept>
#include <string>

class UserError : public std::runtime_error {
public:
    std::string code;
    UserError(const std::string& c, const std::string& msg)
        : std::runtime_error(msg), code(c) {}
};

std::optional<User> authenticate_user(const std::string& username,
    const std::string& password);
User register_user(const std::string& username,
    const std::string& email, const std::string& password,
    const std::string& role);
User change_user_role(int user_id, const std::string& new_role,
    int admin_id);

#endif
