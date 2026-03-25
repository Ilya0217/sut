/*
Модуль: changes_api.cpp
Назначение: Реализация REST API базовых версий и запросов на изменение
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений (try/catch) во всех обработчиках
Требования: FR-11, Functional.ConfigurationManagement.BaselineAndChange
*/

#include "changes_api.hpp"
#include "middleware.hpp"
#include "services/change_service.hpp"

/*
Назначение: Преобразование объекта ChangeRequest в JSON
Входные данные: cr — объект запроса на изменение
Выходные данные: JSON представление запроса
*/
static json cr_to_json(const ChangeRequest& cr)
{
    return {
        {"id", cr.id}, {"requirement_id", cr.requirement_id},
        {"requirement_system_id", cr.requirement_system_id},
        {"status", cr.status}, {"justification", cr.justification},
        {"changes_description", cr.changes_description},
        {"requested_by", cr.requester_username},
        {"assigned_to", cr.assignee_username.empty()
            ? json(nullptr) : json(cr.assignee_username)},
        {"created_at", cr.created_at},
        {"resolved_at", cr.resolved_at.empty()
            ? json(nullptr) : json(cr.resolved_at)},
        {"resolution_comment", cr.resolution_comment.empty()
            ? json(nullptr) : json(cr.resolution_comment)}
    };
}

/*
Назначение: Обработка GET /api/projects/:pid/baselines — список базовых версий
Входные данные: путь с id проекта
Выходные данные: JSON массив базовых версий
*/
static void handle_list_baselines(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int pid = std::stoi(req.matches[1]);
    auto baselines = Database::instance().get_baselines(pid);
    json arr = json::array();
    for (const auto& bl : baselines) {
        arr.push_back(json{
            {"id", bl.id}, {"name", bl.name},
            {"description", bl.description},
            {"created_at", bl.created_at},
            {"requirements_count", bl.requirements_count}});
    }
    json_response(res, arr);
}

/*
Назначение: Обработка POST /api/projects/:pid/baselines — создание базовой версии
Входные данные: JSON {name, requirement_ids?, description?}
Выходные данные: JSON объект базовой версии (201)
*/
static void handle_create_baseline(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int pid = std::stoi(req.matches[1]);
    auto body = parse_json_body(req);
    std::string name = body.value("name", "");

    std::vector<int> req_ids;
    if (body.contains("requirement_ids") && body["requirement_ids"].is_array()) {
        for (const auto& v : body["requirement_ids"]) {
            req_ids.push_back(v.get<int>());
        }
    }

    if (name.empty()) {
        json_error(res, "Название обязательно", 400);
        return;
    }
    if (req_ids.empty()) {
        json_error(res, "Выберите хотя бы одно требование", 400);
        return;
    }
    auto bl = Database::instance().create_baseline(
        pid, name, body.value("description", ""),
        user->id, req_ids);
    json_response(res, json{
        {"id", bl.id}, {"name", bl.name},
        {"description", bl.description},
        {"created_at", bl.created_at},
        {"requirements_count", bl.requirements_count}
    }, 201);
}

/*
Назначение: Обработка GET /api/projects/:pid/change_requests — список запросов
Входные данные: query params: status, page
Выходные данные: JSON {items, total, pages, page}
*/
static void handle_list_change_requests(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int pid = std::stoi(req.matches[1]);
    int page = get_int_param(req, "page", 1);
    auto result = Database::instance().get_change_requests(
        pid, get_str_param(req, "status"), page, 50);

    json items = json::array();
    for (const auto& cr : result.items) {
        items.push_back(cr_to_json(cr));
    }
    json_response(res, {
        {"items", items}, {"total", result.total},
        {"pages", result.pages}, {"page", result.page}
    });
}

/*
Назначение: Обработка POST /api/projects/:pid/change_requests — создание запроса
Входные данные: JSON {requirement_id, justification, changes_description, assigned_to?}
Выходные данные: JSON объект запроса (201)
*/
static void handle_create_change_request(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    auto body = parse_json_body(req);
    try {
        auto cr = create_change_request_svc(
            body.value("requirement_id", 0), user->id,
            body.value("justification", ""),
            body.value("changes_description", ""),
            body.value("assigned_to", 0));
        json_response(res, cr_to_json(cr), 201);
    } catch (const ChangeError& e) {
        json_error(res, e.what(), 400);
    }
}

/*
Назначение: Обработка POST .../approve — одобрение запроса на изменение
Входные данные: JSON {comment?}
Выходные данные: JSON обновлённого запроса
*/
static void handle_approve(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;
    if (!user->can_approve_changes()) {
        json_error(res, "Недостаточно прав", 403);
        return;
    }

    int crid = std::stoi(req.matches[2]);
    auto body = parse_json_body(req);
    try {
        auto cr = approve_change_request_svc(
            crid, user->id, body.value("comment", ""));
        json_response(res, cr_to_json(cr));
    } catch (const ChangeError& e) {
        json_error(res, e.what(), 400);
    }
}

/*
Назначение: Обработка POST .../reject — отклонение запроса на изменение
Входные данные: JSON {comment?}
Выходные данные: JSON обновлённого запроса
*/
static void handle_reject(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;
    if (!user->can_approve_changes()) {
        json_error(res, "Недостаточно прав", 403);
        return;
    }

    int crid = std::stoi(req.matches[2]);
    auto body = parse_json_body(req);
    try {
        auto cr = reject_change_request_svc(
            crid, user->id, body.value("comment", ""));
        json_response(res, cr_to_json(cr));
    } catch (const ChangeError& e) {
        json_error(res, e.what(), 400);
    }
}

void register_change_routes(httplib::Server& svr)
{
    svr.Get(R"(/api/projects/(\d+)/baselines)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_list_baselines(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/baselines)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_create_baseline(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Get(R"(/api/projects/(\d+)/change_requests)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_list_change_requests(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/change_requests)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_create_change_request(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/change_requests/(\d+)/approve)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_approve(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/change_requests/(\d+)/reject)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_reject(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });
}
