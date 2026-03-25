/*
Модуль: requirements_api.cpp
Назначение: Реализация REST API управления требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений (try/catch) во всех обработчиках
Требования: Interface.Software.Requirements, FR-01, FR-02, FR-03, FR-04
*/

#include "requirements_api.hpp"
#include "middleware.hpp"
#include "services/requirement_service.hpp"
#include "services/traceability_service.hpp"

/*
Назначение: Обработка GET /api/projects/:pid/requirements — список с фильтрацией
Входные данные: query params: page, per_page, q, status, priority, category, sort_by, sort_order
Выходные данные: JSON {items, total, pages, page, per_page}
*/
static void handle_list_requirements(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int pid = std::stoi(req.matches[1]);
    int page = get_int_param(req, "page", 1);
    int per_page = get_int_param(req, "per_page", 50);
    std::string q = get_str_param(req, "q");

    Paginated<Requirement> result;
    if (!q.empty()) {
        result = Database::instance().search_requirements(pid, q, page, per_page);
    } else {
        std::string sort_by = get_str_param(req, "sort_by");
        std::string sort_order = get_str_param(req, "sort_order");
        result = Database::instance().list_requirements(
            pid, get_str_param(req, "status"),
            get_str_param(req, "priority"),
            get_str_param(req, "category"),
            sort_by.empty() ? "updated_at" : sort_by,
            sort_order.empty() ? "desc" : sort_order,
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
}

/*
Назначение: Обработка POST /api/projects/:pid/requirements — создание требования
Входные данные: JSON с полями требования
Выходные данные: JSON объект требования (201)
*/
static void handle_create_requirement(const httplib::Request& req, httplib::Response& res)
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
}

/*
Назначение: Обработка GET /api/projects/:pid/requirements/:rid — получение требования
Входные данные: путь с id проекта и требования
Выходные данные: JSON объект требования или 404
*/
static void handle_get_requirement(const httplib::Request& req, httplib::Response& res)
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
}

/*
Назначение: Обработка PUT /api/projects/:pid/requirements/:rid — редактирование
Входные данные: JSON с обновляемыми полями
Выходные данные: JSON обновлённого требования
*/
static void handle_update_requirement(const httplib::Request& req, httplib::Response& res)
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
        edit_requirement(rid, body, user->id);
        check_integrity(rid, user->id);
        auto updated = Database::instance().find_requirement_by_id(rid);
        json_response(res, requirement_to_json(updated.value()));
    } catch (const RequirementError& e) {
        json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
    }
}

/*
Назначение: Обработка DELETE /api/projects/:pid/requirements/:rid — мягкое удаление
Входные данные: путь с id
Выходные данные: JSON {message}
*/
static void handle_soft_delete(const httplib::Request& req, httplib::Response& res)
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
}

/*
Назначение: Обработка DELETE .../hard_delete — безвозвратное удаление
Входные данные: путь с id
Выходные данные: JSON {message}
*/
static void handle_hard_delete(const httplib::Request& req, httplib::Response& res)
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
}

/*
Назначение: Обработка POST .../restore — восстановление удалённого требования
Входные данные: путь с id
Выходные данные: JSON восстановленного требования
*/
static void handle_restore(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int rid = std::stoi(req.matches[2]);
    try {
        restore_requirement(rid, user->id);
        auto updated = Database::instance().find_requirement_by_id(rid);
        json_response(res, requirement_to_json(updated.value()));
    } catch (const RequirementError& e) {
        json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
    }
}

/*
Назначение: Обработка GET .../history — история изменений требования
Входные данные: query params: page
Выходные данные: JSON {items, total, pages, page}
*/
static void handle_history(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int rid = std::stoi(req.matches[2]);
    int page = get_int_param(req, "page", 1);
    auto result = Database::instance().get_requirement_history(rid, page, 50);

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
}

void register_requirement_routes(httplib::Server& svr)
{
    svr.Get(R"(/api/projects/(\d+)/requirements)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_list_requirements(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/requirements)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_create_requirement(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Get(R"(/api/projects/(\d+)/requirements/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_get_requirement(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Put(R"(/api/projects/(\d+)/requirements/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_update_requirement(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Delete(R"(/api/projects/(\d+)/requirements/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_soft_delete(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Delete(R"(/api/projects/(\d+)/requirements/(\d+)/hard_delete)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_hard_delete(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/requirements/(\d+)/restore)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_restore(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Get(R"(/api/projects/(\d+)/requirements/(\d+)/history)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_history(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });
}
