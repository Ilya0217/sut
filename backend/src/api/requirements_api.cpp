/*
Модуль: requirements_api.cpp
Назначение: Реализация REST API управления требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Requirements, FR-01-FR-04
*/

#include "requirements_api.hpp"
#include "middleware.hpp"
#include "services/requirement_service.hpp"
#include "services/traceability_service.hpp"

void register_requirement_routes(httplib::Server& svr)
{
    svr.Get(R"(/api/projects/(\d+)/requirements)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int pid = std::stoi(req.matches[1]);
        int page = get_int_param(req, "page", 1);
        int per_page = get_int_param(req, "per_page", 50);
        std::string q = get_str_param(req, "q");

        Paginated<Requirement> result;
        if (!q.empty()) {
            result = Database::instance().search_requirements(
                pid, q, page, per_page);
        } else {
            result = Database::instance().list_requirements(
                pid, get_str_param(req, "status"),
                get_str_param(req, "priority"),
                get_str_param(req, "category"),
                get_str_param(req, "sort_by").empty()
                    ? "updated_at" : get_str_param(req, "sort_by"),
                get_str_param(req, "sort_order").empty()
                    ? "desc" : get_str_param(req, "sort_order"),
                page, per_page,
                get_str_param(req, "include_deleted") == "1");
        }

        json items = json::array();
        for (const auto& r : result.items) {
            items.push_back(requirement_to_json(r));
        }
        json_response(res, {
            {"items", items}, {"total", result.total},
            {"pages", result.pages}, {"page", result.page},
            {"per_page", result.per_page}
        });
    });

    svr.Post(R"(/api/projects/(\d+)/requirements)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        if (!user->can_create()) {
            json_error(res, "Недостаточно прав", 403);
            return;
        }
        int pid = std::stoi(req.matches[1]);
        auto body = parse_json_body(req);
        try {
            auto r = create_requirement(pid, body, user->id);
            json_response(res, requirement_to_json(r), 201);
        } catch (const RequirementError& e) {
            json_response(res, {
                {"error", e.what()}, {"code", e.code}, {"field", e.field}
            }, 400);
        }
    });

    svr.Get(R"(/api/projects/(\d+)/requirements/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int rid = std::stoi(req.matches[2]);
        auto r = Database::instance().find_requirement_by_id(rid);
        if (!r.has_value()) {
            json_error(res, "Не найдено", 404);
            return;
        }
        json_response(res, requirement_to_json(r.value()));
    });

    svr.Put(R"(/api/projects/(\d+)/requirements/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        if (!user->can_edit()) {
            json_error(res, "Недостаточно прав", 403);
            return;
        }
        int rid = std::stoi(req.matches[2]);
        auto body = parse_json_body(req);
        try {
            auto r = edit_requirement(rid, body, user->id);
            check_integrity(rid, user->id);
            auto updated = Database::instance().find_requirement_by_id(rid);
            json_response(res, requirement_to_json(updated.value()));
        } catch (const RequirementError& e) {
            json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
        }
    });

    svr.Delete(R"(/api/projects/(\d+)/requirements/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        if (!user->can_delete()) {
            json_error(res, "Недостаточно прав", 403);
            return;
        }
        int rid = std::stoi(req.matches[2]);
        try {
            soft_delete_requirement(rid, user->id);
            check_integrity(rid, user->id);
            json_response(res, {{"message", "Удалено"}});
        } catch (const RequirementError& e) {
            json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
        }
    });

    svr.Delete(R"(/api/projects/(\d+)/requirements/(\d+)/hard_delete)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int rid = std::stoi(req.matches[2]);
        try {
            hard_delete_requirement(rid, user->id);
            json_response(res, {{"message", "Удалено безвозвратно"}});
        } catch (const RequirementError& e) {
            json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/requirements/(\d+)/restore)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int rid = std::stoi(req.matches[2]);
        try {
            auto r = restore_requirement(rid, user->id);
            auto updated = Database::instance().find_requirement_by_id(rid);
            json_response(res, requirement_to_json(updated.value()));
        } catch (const RequirementError& e) {
            json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
        }
    });

    svr.Get(R"(/api/projects/(\d+)/requirements/(\d+)/history)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int rid = std::stoi(req.matches[2]);
        int page = get_int_param(req, "page", 1);
        auto result = Database::instance().get_requirement_history(
            rid, page, 50);

        json items = json::array();
        for (const auto& h : result.items) {
            items.push_back({
                {"id", h.id}, {"event_type", h.event_type},
                {"attribute_name", h.attribute_name},
                {"old_value", h.old_value},
                {"new_value", h.new_value},
                {"user", h.username.empty() ? json(nullptr) : json(h.username)},
                {"created_at", h.created_at}
            });
        }
        json_response(res, {
            {"items", items}, {"total", result.total},
            {"pages", result.pages}, {"page", result.page}
        });
    });
}
