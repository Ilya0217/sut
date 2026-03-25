/*
Модуль: session_manager.hpp
Назначение: Управление пользовательскими сессиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: LLR_SecurityManager_Authenticate_01
*/

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "models.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class SessionManager {
public:
    static SessionManager& instance();

    std::string create_session(int user_id);
    void destroy_session(const std::string& token);
    std::optional<int> get_user_id(const std::string& token) const;
    std::string extract_token(const std::string& cookie_header) const;

private:
    SessionManager() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, int> sessions_;
};

#endif
