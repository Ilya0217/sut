/*
Модуль: session_manager.cpp
Назначение: Реализация управления сессиями с истечением срока (24 часа)
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена проверка истечения сессий
Требования: LLR_SecurityManager_Authenticate_01, NFR-03
*/

#include "session_manager.hpp"
#include "password_utils.hpp"

SessionManager& SessionManager::instance()
{
    static SessionManager mgr;
    return mgr;
}

/*
Назначение: Создание новой сессии для пользователя
Входные данные: user_id — идентификатор пользователя
Выходные данные: сгенерированный токен сессии
*/
std::string SessionManager::create_session(int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string token = generate_session_token();
    sessions_[token] = SessionData{user_id, std::chrono::steady_clock::now()};
    return token;
}

/*
Назначение: Уничтожение сессии по токену
Входные данные: token — токен сессии
Выходные данные: нет
*/
void SessionManager::destroy_session(const std::string& token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(token);
}

/*
Назначение: Поиск user_id по токену с проверкой срока действия
Входные данные: token — токен сессии
Выходные данные: user_id или nullopt (сессия не найдена / истекла)
*/
std::optional<int> SessionManager::get_user_id(const std::string& token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    if (is_expired(it->second)) {
        sessions_.erase(it);
        return std::nullopt;
    }
    return it->second.user_id;
}

/*
Назначение: Извлечение токена из строки заголовка Cookie
Входные данные: cookie_header — значение заголовка Cookie
Выходные данные: токен сессии или пустая строка
*/
std::string SessionManager::extract_token(const std::string& cookie_header) const
{
    const std::string prefix = "session=";
    auto pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        return "";
    }
    auto start = pos + prefix.size();
    auto end = cookie_header.find(';', start);
    if (end == std::string::npos) {
        return cookie_header.substr(start);
    }
    return cookie_header.substr(start, end - start);
}

/*
Назначение: Проверка истечения срока сессии (24 часа)
Входные данные: data — данные сессии
Выходные данные: true если с момента создания прошло более 24 часов
*/
bool SessionManager::is_expired(const SessionData& data) const
{
    auto elapsed = std::chrono::steady_clock::now() - data.created_at;
    return elapsed > SESSION_TTL;
}
