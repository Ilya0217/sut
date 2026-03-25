/*
Модуль: session_manager.cpp
Назначение: Реализация управления сессиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: LLR_SecurityManager_Authenticate_01
*/

#include "session_manager.hpp"
#include "password_utils.hpp"

SessionManager& SessionManager::instance()
{
    static SessionManager mgr;
    return mgr;
}

/*
Назначение: Создание новой сессии
Входные данные: user_id — идентификатор пользователя
Выходные данные: токен сессии
*/
std::string SessionManager::create_session(int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string token = generate_session_token();
    sessions_[token] = user_id;
    return token;
}

/*
Назначение: Уничтожение сессии
Входные данные: token — токен сессии
Выходные данные: нет
*/
void SessionManager::destroy_session(const std::string& token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(token);
}

/*
Назначение: Получение user_id по токену
Входные данные: token — токен сессии
Выходные данные: user_id или пусто
*/
std::optional<int> SessionManager::get_user_id(const std::string& token) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(token);
    if (it == sessions_.end()) return std::nullopt;
    return it->second;
}

/*
Назначение: Извлечение токена из заголовка Cookie
Входные данные: cookie_header — строка заголовка
Выходные данные: значение токена сессии
*/
std::string SessionManager::extract_token(const std::string& cookie_header) const
{
    const std::string prefix = "session=";
    auto pos = cookie_header.find(prefix);
    if (pos == std::string::npos) return "";

    auto start = pos + prefix.size();
    auto end = cookie_header.find(';', start);
    if (end == std::string::npos) {
        return cookie_header.substr(start);
    }
    return cookie_header.substr(start, end - start);
}
