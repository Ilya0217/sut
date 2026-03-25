/*
Модуль: middleware.hpp
Назначение: Общие утилиты для REST API обработчиков
Автор: Разработчик
Дата создания: 21.03.2026
Требования: NFR-03, LLR_SecurityManager_Authenticate_01
*/

#ifndef MIDDLEWARE_HPP
#define MIDDLEWARE_HPP

#include "database.hpp"
#include "session_manager.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <optional>

using json = nlohmann::json;

/*
Назначение: Отправка JSON ответа
Входные данные: объект ответа, JSON данные, HTTP код
Выходные данные: нет
*/
inline void json_response(httplib::Response& res,
    const json& data, int status = 200)
{
    res.status = status;
    res.set_content(data.dump(), "application/json");
}

/*
Назначение: Отправка ошибки в JSON формате
Входные данные: объект ответа, сообщение, HTTP код
Выходные данные: нет
*/
inline void json_error(httplib::Response& res,
    const std::string& message, int status = 400)
{
    json_response(res, {{"error", message}}, status);
}

/*
Назначение: Получение текущего пользователя из сессии
Входные данные: HTTP запрос
Выходные данные: объект User или nullopt
*/
inline std::optional<User> get_current_user(const httplib::Request& req)
{
    auto cookie_it = req.headers.find("Cookie");
    if (cookie_it == req.headers.end()) return std::nullopt;

    auto& sm = SessionManager::instance();
    std::string token = sm.extract_token(cookie_it->second);
    if (token.empty()) return std::nullopt;

    auto uid = sm.get_user_id(token);
    if (!uid.has_value()) return std::nullopt;

    return Database::instance().find_user_by_id(uid.value());
}

/*
Назначение: Проверка аутентификации с отправкой ошибки
Входные данные: запрос, ответ
Выходные данные: пользователь или nullopt (с отправкой 401)
*/
inline std::optional<User> require_auth(
    const httplib::Request& req, httplib::Response& res)
{
    auto user = get_current_user(req);
    if (!user.has_value()) {
        json_error(res, "Требуется аутентификация", 401);
    }
    return user;
}

/*
Назначение: Извлечение JSON из тела запроса
Входные данные: HTTP запрос
Выходные данные: JSON объект
*/
inline json parse_json_body(const httplib::Request& req)
{
    if (req.body.empty()) return json::object();
    try {
        return json::parse(req.body);
    } catch (...) {
        return json::object();
    }
}

/*
Назначение: Извлечение числового параметра из query string
Входные данные: запрос, имя параметра, значение по умолчанию
Выходные данные: числовое значение
*/
inline int get_int_param(const httplib::Request& req,
    const std::string& name, int default_val)
{
    if (!req.has_param(name)) return default_val;
    try {
        return std::stoi(req.get_param_value(name));
    } catch (...) {
        return default_val;
    }
}

/*
Назначение: Извлечение строкового параметра из query string
Входные данные: запрос, имя параметра
Выходные данные: строковое значение
*/
inline std::string get_str_param(const httplib::Request& req,
    const std::string& name)
{
    if (!req.has_param(name)) return "";
    return req.get_param_value(name);
}

/*
Назначение: Преобразование требования в JSON
Входные данные: объект Requirement
Выходные данные: JSON объект
*/
inline json requirement_to_json(const Requirement& r)
{
    return {
        {"id", r.id}, {"system_id", r.system_id},
        {"custom_id", r.custom_id.empty() ? json(nullptr) : json(r.custom_id)},
        {"project_id", r.project_id},
        {"title", r.title}, {"text", r.text},
        {"category", r.category}, {"priority", r.priority},
        {"status", r.status},
        {"parent_id", r.parent_id > 0 ? json(r.parent_id) : json(nullptr)},
        {"responsible_user_id", r.responsible_user_id > 0
            ? json(r.responsible_user_id) : json(nullptr)},
        {"responsible_user", r.responsible_username.empty()
            ? json(nullptr) : json(r.responsible_username)},
        {"version", r.version}, {"is_baseline", r.is_baseline},
        {"is_deleted", r.is_deleted},
        {"created_at", r.created_at}, {"updated_at", r.updated_at},
        {"created_by", r.creator_username.empty()
            ? json(nullptr) : json(r.creator_username)},
    };
}

#endif
