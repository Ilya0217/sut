/*
Модуль: users_api.cpp
Назначение: Реализация REST API управления пользователями
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений (try/catch) во всех обработчиках
Требования: Functional.Security.UserManagement
*/

#include "users_api.hpp"
#include "middleware.hpp"
#include "services/user_service.hpp"

/*
Назначение: Обработка GET /api/users — список всех пользователей
Входные данные: cookie session (авторизованный пользователь)
Выходные данные: JSON массив пользователей
*/
static void handle_list_users(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    auto users = Database::instance().get_all_users();
    json arr = json::array();
    for (const auto& u : users) {
        arr.push_back({
            {"id", u.id}, {"username", u.username},
            {"email", u.email}, {"role", u.role},
            {"created_at", u.created_at}
        });
    }
    json_response(res, arr);
}

/*
Назначение: Обработка PUT /api/users/:id/role — изменение роли пользователя
Входные данные: JSON {role}
Выходные данные: JSON {id, username, role}
*/
static void handle_change_role(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;
    if (!user->can_manage_users()) {
        json_error(res, "Доступ запрещён", 403);
        return;
    }

    int uid = std::stoi(req.matches[1]);
    auto body = parse_json_body(req);
    try {
        auto updated = change_user_role(
            uid, body.value("role", ""), user->id);
        json_response(res, {
            {"id", updated.id}, {"username", updated.username},
            {"role", updated.role}
        });
    } catch (const UserError& e) {
        json_error(res, e.what(), 400);
    }
}

void register_user_routes(httplib::Server& svr)
{
    svr.Get("/api/users", [](const httplib::Request& req,
        httplib::Response& res)
    {
        try {
            handle_list_users(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Put(R"(/api/users/(\d+)/role)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_change_role(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });
}
