/*
Модуль: user_service.cpp
Назначение: Реализация сервиса управления пользователями
Автор: Разработчик
Дата создания: 25.03.2026
Требования: Functional.Security.UserManagement
*/

#include "user_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "password_utils.hpp"

std::optional<User> authenticate_user(const std::string& username,
    const std::string& password)
{
    auto user = Database::instance().find_user_by_username(username);
    if (!user.has_value()) return std::nullopt;
    if (!check_password(password, user->password_hash)) {
        return std::nullopt;
    }
    return user;
}

User register_user(const std::string& username,
    const std::string& email, const std::string& password,
    const std::string& role)
{
    if (username.empty() || email.empty() || password.empty()) {
        throw UserError("ERR_MISSING_FIELDS",
            "Все поля обязательны");
    }
    if (!is_valid(role, VALID_ROLES)) {
        throw UserError("ERR_INVALID_ROLE",
            "Недопустимая роль");
    }
    auto existing = Database::instance()
        .find_user_by_username(username);
    if (existing.has_value()) {
        throw UserError("ERR_DUPLICATE_USERNAME",
            "Пользователь уже существует");
    }

    std::string pw_hash = hash_password(password);
    auto user = Database::instance().create_user(
        username, email, pw_hash, role);
    log_action(user.id, "register", "user",
        std::to_string(user.id));
    return user;
}

User change_user_role(int user_id, const std::string& new_role,
    int admin_id)
{
    if (!is_valid(new_role, VALID_ROLES)) {
        throw UserError("ERR_INVALID_ROLE",
            "Недопустимая роль");
    }
    auto user = Database::instance().find_user_by_id(user_id);
    if (!user.has_value()) {
        throw UserError("ERR_USER_NOT_FOUND",
            "Пользователь не найден");
    }
    std::string old_role = user->role;
    Database::instance().update_user_role(user_id, new_role);
    log_action(admin_id, "change_role", "user",
        std::to_string(user_id), old_role, new_role);
    user->role = new_role;
    return user.value();
}
