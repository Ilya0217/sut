/*
Модуль: auth_api.cpp
Назначение: Реализация REST API аутентификации
Автор: Разработчик
Дата создания: 21.03.2026
Требования: LLR_SecurityManager_Authenticate_01
*/

#include "auth_api.hpp"
#include "middleware.hpp"
#include "session_manager.hpp"
#include "services/user_service.hpp"

void register_auth_routes(httplib::Server& svr)
{
    svr.Post("/api/auth/login", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto body = parse_json_body(req);
        std::string username = body.value("username", "");
        std::string password = body.value("password", "");

        if (username.empty() || password.empty()) {
            json_error(res, "Имя пользователя и пароль обязательны", 400);
            return;
        }

        auto user = authenticate_user(username, password);
        if (!user.has_value()) {
            json_error(res, "Неверное имя пользователя или пароль", 401);
            return;
        }

        auto token = SessionManager::instance().create_session(user->id);
        res.set_header("Set-Cookie",
            "session=" + token + "; Path=/; HttpOnly; SameSite=Lax");
        json_response(res, {
            {"id", user->id}, {"username", user->username},
            {"email", user->email}, {"role", user->role}
        });
    });

    svr.Post("/api/auth/register", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto body = parse_json_body(req);
        try {
            auto user = register_user(
                body.value("username", ""),
                body.value("email", ""),
                body.value("password", ""),
                body.value("role", "analyst"));
            auto token = SessionManager::instance().create_session(user.id);
            res.set_header("Set-Cookie",
                "session=" + token + "; Path=/; HttpOnly; SameSite=Lax");
            json_response(res, {
                {"id", user.id}, {"username", user.username},
                {"email", user.email}, {"role", user.role}
            }, 201);
        } catch (const UserError& e) {
            json_error(res, e.what(), 400);
        }
    });

    svr.Post("/api/auth/logout", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto cookie_it = req.headers.find("Cookie");
        if (cookie_it != req.headers.end()) {
            auto& sm = SessionManager::instance();
            std::string token = sm.extract_token(cookie_it->second);
            sm.destroy_session(token);
        }
        res.set_header("Set-Cookie",
            "session=; Path=/; HttpOnly; Max-Age=0");
        json_response(res, {{"message", "OK"}});
    });

    svr.Get("/api/auth/me", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        json_response(res, {
            {"id", user->id}, {"username", user->username},
            {"email", user->email}, {"role", user->role}
        });
    });
}
