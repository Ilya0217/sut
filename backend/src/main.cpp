/*
Модуль: main.cpp
Назначение: Точка входа веб-приложения СУТ (C++ бэкенд)
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — пароль из конфигурации, обработка исключений, стандарт кодирования
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
Назначение: Создание или обновление администратора по умолчанию
Входные данные: admin_password — пароль из конфигурации (Config::default_admin_password)
Выходные данные: нет
*/
static void create_default_admin(const std::string& admin_password)
{
    auto admin = Database::instance().find_user_by_username("admin");
    std::string pw_hash = hash_password(admin_password);

    if (!admin.has_value()) {
        Database::instance().create_user(
            "admin", "admin@sut.local", pw_hash, "admin");
        std::cout << "Создан администратор: admin" << std::endl;
        return;
    }

    if (!check_password(admin_password, admin->password_hash)) {
        Database::instance().update_user_password(admin->id, pw_hash);
        std::cout << "Обновлён хеш пароля администратора" << std::endl;
    }
}

/*
Назначение: Обработка GET /api/projects/:pid/export/:fmt — экспорт требований
Входные данные: путь с id проекта и форматом (json/csv)
Выходные данные: файл в выбранном формате
*/
static void handle_export(const httplib::Request& req, httplib::Response& res)
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
        res.set_content(export_to_json(pid, user->id), "application/json");
    } else if (fmt == "csv") {
        res.set_content(export_to_csv(pid, user->id), "text/csv");
    } else {
        json_error(res, "Неподдерживаемый формат", 400);
    }
}

/*
Назначение: Обработка POST /api/projects/:pid/import — импорт требований из файла
Входные данные: multipart form с файлом (json или csv)
Выходные данные: JSON результат импорта
*/
static void handle_import(const httplib::Request& req, httplib::Response& res)
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
    nlohmann::json result;
    if (file.filename.find(".json") != std::string::npos) {
        result = import_from_json(file.content, pid, user->id);
    } else if (file.filename.find(".csv") != std::string::npos) {
        result = import_from_csv(file.content, pid, user->id);
    } else {
        json_error(res, "Неподдерживаемый формат", 400);
        return;
    }
    json_response(res, result);
}

/*
Назначение: Регистрация маршрутов импорта/экспорта требований
Входные данные: svr — HTTP сервер
Выходные данные: нет
*/
static void register_import_export_routes(httplib::Server& svr)
{
    svr.Get(R"(/api/projects/(\d+)/export/(\w+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_export(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/import)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_import(req, res);
        } catch (const ImportExportError& e) {
            json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });
}

/*
Назначение: Настройка глобального обработчика исключений сервера
Входные данные: svr — HTTP сервер
Выходные данные: нет
*/
static void setup_exception_handler(httplib::Server& svr)
{
    svr.set_exception_handler(
        [](const httplib::Request&, httplib::Response& res,
            std::exception_ptr ep)
    {
        std::string msg = "Internal Server Error";
        try {
            if (ep) std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            msg = e.what();
            std::cerr << "Ошибка: " << msg << std::endl;
        }
        json_error(res, msg, 500);
    });
}

/*
Назначение: Настройка CORS заголовков и обработки OPTIONS
Входные данные: svr — HTTP сервер, cfg — конфигурация
Выходные данные: нет
*/
static void setup_cors(httplib::Server& svr, const Config& cfg)
{
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
}

/*
Назначение: Регистрация всех маршрутов API
Входные данные: svr — HTTP сервер
Выходные данные: нет
*/
static void register_all_routes(httplib::Server& svr)
{
    svr.Get("/api/health", [](const httplib::Request&,
        httplib::Response& res)
    {
        try {
            json_response(res, {{"status", "ok"}});
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
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
}

int main()
{
    auto cfg = Config::load();

    std::cout << "Инициализация базы данных..." << std::endl;
    Database::instance().initialize(cfg.database_url);
    Database::instance().run_migrations();
    create_default_admin(cfg.default_admin_password);

    httplib::Server svr;

    setup_exception_handler(svr);
    setup_cors(svr, cfg);
    register_all_routes(svr);

    std::cout << "СУТ бэкенд (C++) запущен на порту "
              << cfg.port << std::endl;
    svr.listen("0.0.0.0", cfg.port);

    return 0;
}
