/*
Модуль: projects_api.cpp
Назначение: Реализация REST API управления проектами
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Projects
*/

#include "projects_api.hpp"
#include "middleware.hpp"
#include "services/audit_service.hpp"

void register_project_routes(httplib::Server& svr)
{
    svr.Get("/api/projects", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        auto projects = Database::instance().get_all_projects();
        json arr = json::array();
        for (const auto& p : projects) {
            arr.push_back({
                {"id", p.id}, {"name", p.name},
                {"description", p.description},
                {"created_at", p.created_at},
                {"created_by", p.created_by}
            });
        }
        json_response(res, arr);
    });

    svr.Post("/api/projects", [](const httplib::Request& req,
        httplib::Response& res)
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
        json_response(res, {
            {"id", p.id}, {"name", p.name},
            {"description", p.description},
            {"created_at", p.created_at},
            {"created_by", p.created_by}
        }, 201);
    });

    svr.Get(R"(/api/projects/(\d+))", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int pid = std::stoi(req.matches[1]);
        auto p = Database::instance().find_project_by_id(pid);
        if (!p.has_value()) {
            json_error(res, "Не найдено", 404);
            return;
        }
        json_response(res, {
            {"id", p->id}, {"name", p->name},
            {"description", p->description},
            {"created_at", p->created_at},
            {"created_by", p->created_by}
        });
    });
}
