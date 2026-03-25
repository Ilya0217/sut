/*
Модуль: main.cpp
Назначение: Точка входа веб-приложения СУТ (C++ бэкенд)
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Архитектура ПО (ApiGateway)
*/

#include "config.hpp"
#include "database.hpp"
#include "password_utils.hpp"
#include "session_manager.hpp"

#include "api/auth_api.hpp"
#include "api/projects_api.hpp"
#include "api/requirements_api.hpp"
#include "api/tracelinks_api.hpp"
#include "api/changes_api.hpp"
#include "api/notifications_api.hpp"
#include "api/audit_api.hpp"
#include "api/users_api.hpp"

#include "services/import_export_service.hpp"
#include "api/middleware.hpp"

#include <httplib.h>
#include <iostream>

/*
Назначение: Создание администратора по умолчанию
Входные данные: нет
Выходные данные: нет
*/
/*
Назначение: Создание или обновление администратора по умолчанию
Входные данные: нет
Выходные данные: нет
*/
static void create_default_admin()
{
    auto admin = Database::instance().find_user_by_username("admin");
    std::string pw_hash = hash_password("admin123");

    if (!admin.has_value()) {
        Database::instance().create_user("admin", "admin@sut.local",
            pw_hash, "admin");
        std::cout << "Создан администратор: admin / admin123" << std::endl;
        return;
    }

    if (!check_password("admin123", admin->password_hash)) {
        Database::instance().update_user_password(admin->id, pw_hash);
        std::cout << "Обновлён хеш пароля администратора" << std::endl;
    }
}

/*
Назначение: Регистрация маршрутов импорта/экспорта
Входные данные: сервер
Выходные данные: нет
*/
static void register_import_export_routes(httplib::Server& svr)
{
    svr.Get(R"(/api/projects/(\d+)/export/(\w+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        if (!user->can_import_export()) {
            json_error(res, "Недостаточно прав", 403);
            return;
        }
        int pid = std::stoi(req.matches[1]);
        std::string fmt = req.matches[2];

        if (fmt == "json") {
            std::string content = export_to_json(pid, user->id);
            res.set_content(content, "application/json");
        } else if (fmt == "csv") {
            std::string content = export_to_csv(pid, user->id);
            res.set_content(content, "text/csv");
        } else {
            json_error(res, "Неподдерживаемый формат", 400);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/import)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        if (!user->can_import_export()) {
            json_error(res, "Недостаточно прав", 403);
            return;
        }
        int pid = std::stoi(req.matches[1]);

        if (!req.has_file("file")) {
            json_error(res, "Файл не выбран", 400);
            return;
        }
        auto file = req.get_file_value("file");
        std::string filename = file.filename;
        std::string content = file.content;

        try {
            nlohmann::json result;
            if (filename.find(".json") != std::string::npos) {
                result = import_from_json(content, pid, user->id);
            } else if (filename.find(".csv") != std::string::npos) {
                result = import_from_csv(content, pid, user->id);
            } else {
                json_error(res, "Неподдерживаемый формат", 400);
                return;
            }
            json_response(res, result);
        } catch (const ImportExportError& e) {
            json_response(res,
                {{"error", e.what()}, {"code", e.code}}, 400);
        }
    });
}

int main()
{
    auto cfg = Config::load();

    std::cout << "Инициализация базы данных..." << std::endl;
    Database::instance().initialize(cfg.database_url);
    Database::instance().run_migrations();
    create_default_admin();

    httplib::Server svr;

    svr.set_pre_routing_handler(
        [&cfg](const httplib::Request& req, httplib::Response& res)
        -> httplib::Server::HandlerResponse
    {
        res.set_header("Access-Control-Allow-Origin", cfg.cors_origin);
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods",
            "GET,POST,PUT,DELETE,OPTIONS");

        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Get("/api/health", [](const httplib::Request&,
        httplib::Response& res)
    {
        json_response(res, {{"status", "ok"}});
    });

    register_auth_routes(svr);
    register_project_routes(svr);
    register_requirement_routes(svr);
    register_tracelink_routes(svr);
    register_change_routes(svr);
    register_notification_routes(svr);
    register_audit_routes(svr);
    register_user_routes(svr);
    register_import_export_routes(svr);

    std::cout << "СУТ бэкенд (C++) запущен на порту "
              << cfg.port << std::endl;
    svr.listen("0.0.0.0", cfg.port);

    return 0;
}
