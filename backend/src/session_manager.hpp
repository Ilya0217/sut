/*
Модуль: session_manager.hpp
Назначение: Управление пользовательскими сессиями с поддержкой истечения срока
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлено истечение сессий (24 часа)
Требования: LLR_SecurityManager_Authenticate_01, NFR-03
*/

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include "models.hpp"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

/* Данные одной сессии: идентификатор пользователя и время создания */
struct SessionData {
    int user_id = 0;
    std::chrono::steady_clock::time_point created_at;
};

/*
Назначение: Потокобезопасный менеджер сессий (singleton)
Хранит активные сессии в памяти. Сессии автоматически истекают через 24 часа.
*/
class SessionManager {
public:
    /* Время жизни сессии: 24 часа */
    static constexpr auto SESSION_TTL = std::chrono::hours(24);

    static SessionManager& instance();

    /*
    Назначение: Создание новой сессии
    Входные данные: user_id — идентификатор пользователя
    Выходные данные: токен сессии (строка)
    */
    std::string create_session(int user_id);

    /*
    Назначение: Уничтожение сессии
    Входные данные: token — токен сессии
    Выходные данные: нет
    */
    void destroy_session(const std::string& token);

    /*
    Назначение: Получение user_id по токену (с проверкой срока)
    Входные данные: token — токен сессии
    Выходные данные: user_id или nullopt если сессия не найдена / истекла
    */
    std::optional<int> get_user_id(const std::string& token);

    /*
    Назначение: Извлечение токена из заголовка Cookie
    Входные данные: cookie_header — строка заголовка
    Выходные данные: значение токена сессии
    */
    std::string extract_token(const std::string& cookie_header) const;

private:
    SessionManager() = default;

    /*
    Назначение: Проверка истечения срока сессии
    Входные данные: данные сессии
    Выходные данные: true если сессия истекла
    */
    bool is_expired(const SessionData& data) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, SessionData> sessions_;
};

#endif
