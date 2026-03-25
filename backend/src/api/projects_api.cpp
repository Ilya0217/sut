/*
Модуль: projects_api.cpp
Назначение: Реализация REST API управления проектами
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений (try/catch) во всех обработчиках
Требования: Interface.Software.Projects
*/

#include "projects_api.hpp"
#include "middleware.hpp"
#include "services/audit_service.hpp"

/*
Назначение: Преобразование объекта проекта в JSON
Входные данные: p — объект Project
Выходные данные: JSON представление проекта
*/
static json project_to_json(const Project& p)
{
    return {
        {"id", p.id}, {"name", p.name},
        {"description", p.description},
        {"created_at", p.created_at},
        {"created_by", p.created_by}
    };
}

/*
Назначение: Обработка GET /api/projects — список всех проектов
Входные данные: HTTP запрос с аутентификацией
Выходные данные: JSON массив проектов
*/
static void handle_list_projects(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    auto projects = Database::instance().get_all_projects();
    json arr = json::array();
    for (const auto& p : projects) {
        arr.push_back(project_to_json(p));
    }
    json_response(res, arr);
}

/*
Назначение: Обработка POST /api/projects — создание проекта
Входные данные: JSON {name, description?}
Выходные данные: JSON объект проекта (201)
*/
static void handle_create_project(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    auto body = parse_json_body(req);
    std::string name = body.value("name", "");
    if (name.empty()) {
        json_error(res, "Название проекта обязательно", 400);
        return;
    }

    auto p = Database::instance().create_project(
        name, body.value("description", ""), user->id);
    log_action(user->id, "create", "project",
        std::to_string(p.id), "", name);
    json_response(res, project_to_json(p), 201);
}

/*
Назначение: Обработка GET /api/projects/:id — получение проекта
Входные данные: путь с id проекта
Выходные данные: JSON объект проекта или 404
*/
static void handle_get_project(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int pid = std::stoi(req.matches[1]);
    auto p = Database::instance().find_project_by_id(pid);
    if (!p.has_value()) {
        json_error(res, "Не найдено", 404);
        return;
    }
    json_response(res, project_to_json(p.value()));
}

void register_project_routes(httplib::Server& svr)
{
    svr.Get("/api/projects", [](const httplib::Request& req,
        httplib::Response& res)
    {
        try {
            handle_list_projects(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post("/api/projects", [](const httplib::Request& req,
        httplib::Response& res)
    {
        try {
            handle_create_project(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Get(R"(/api/projects/(\d+))", [](const httplib::Request& req,
        httplib::Response& res)
    {
        try {
            handle_get_project(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });
}
