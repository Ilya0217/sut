/*
Модуль: tracelinks_api.cpp
Назначение: Реализация REST API связей трассировки между требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений (try/catch) во всех обработчиках
Требования: Interface.Software.TraceLinks, FR-05, FR-06
*/

#include "tracelinks_api.hpp"
#include "middleware.hpp"
#include "services/traceability_service.hpp"

/*
Назначение: Преобразование объекта TraceLink в JSON
Входные данные: l — объект связи трассировки
Выходные данные: JSON представление связи
*/
static json link_to_json(const TraceLink& l)
{
    return {
        {"id", l.id},
        {"source_req_id", l.source_req_id},
        {"source_system_id", l.source_system_id},
        {"source_title", l.source_title},
        {"target_req_id", l.target_req_id},
        {"target_system_id", l.target_system_id},
        {"target_title", l.target_title},
        {"link_type", l.link_type},
        {"description", l.description},
        {"status", l.status},
        {"created_at", l.created_at},
        {"created_by", l.creator_username.empty()
            ? json(nullptr) : json(l.creator_username)}
    };
}

/*
Назначение: Обработка GET /api/projects/:pid/tracelinks — список связей проекта
Входные данные: путь с id проекта
Выходные данные: JSON массив связей
*/
static void handle_list_links(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int pid = std::stoi(req.matches[1]);
    auto links = Database::instance().get_links_for_project(pid);
    json arr = json::array();
    for (const auto& l : links) {
        arr.push_back(link_to_json(l));
    }
    json_response(res, arr);
}

/*
Назначение: Обработка POST /api/projects/:pid/tracelinks — создание связи
Входные данные: JSON {source_req_id, target_req_id, link_type?, description?}
Выходные данные: JSON объект связи (201)
*/
static void handle_create_link(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;
    if (!user->can_manage_links()) {
        json_error(res, "Недостаточно прав", 403);
        return;
    }

    auto body = parse_json_body(req);
    try {
        auto link = create_trace_link(
            body.value("source_req_id", 0),
            body.value("target_req_id", 0),
            body.value("link_type", "derives_from"),
            user->id, body.value("description", ""));
        json_response(res, link_to_json(link), 201);
    } catch (const TraceLinkError& e) {
        json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
    }
}

/*
Назначение: Обработка DELETE /api/projects/:pid/tracelinks/:lid — удаление связи
Входные данные: путь с id связи
Выходные данные: JSON {message}
*/
static void handle_delete_link(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;
    if (!user->can_manage_links()) {
        json_error(res, "Недостаточно прав", 403);
        return;
    }

    int lid = std::stoi(req.matches[2]);
    try {
        delete_trace_link_svc(lid, user->id);
        json_response(res, {{"message", "Удалено"}});
    } catch (const TraceLinkError& e) {
        json_response(res, {{"error", e.what()}, {"code", e.code}}, 400);
    }
}

/*
Назначение: Обработка GET .../tracelinks/requirement/:rid — связи требования
Входные данные: путь с id требования
Выходные данные: JSON массив связей
*/
static void handle_links_for_requirement(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;

    int rid = std::stoi(req.matches[2]);
    auto links = Database::instance().get_links_for_requirement(rid);
    json arr = json::array();
    for (const auto& l : links) {
        arr.push_back(link_to_json(l));
    }
    json_response(res, arr);
}

/*
Назначение: Обработка POST .../tracelinks/integrity_check — проверка целостности
Входные данные: путь с id проекта
Выходные данные: JSON {issues_count, issues}
*/
static void handle_integrity_check(const httplib::Request& req, httplib::Response& res)
{
    auto user = require_auth(req, res);
    if (!user) return;
    if (!user->can_run_integrity_check()) {
        json_error(res, "Недостаточно прав", 403);
        return;
    }

    int pid = std::stoi(req.matches[1]);
    auto issues = run_full_integrity_check(pid, user->id);
    json arr = json::array();
    for (const auto& issue : issues) {
        arr.push_back(issue);
    }
    json_response(res, json{
        {"issues_count", static_cast<int>(issues.size())},
        {"issues", arr}});
}

void register_tracelink_routes(httplib::Server& svr)
{
    svr.Get(R"(/api/projects/(\d+)/tracelinks)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_list_links(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/tracelinks)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_create_link(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Delete(R"(/api/projects/(\d+)/tracelinks/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_delete_link(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Get(R"(/api/projects/(\d+)/tracelinks/requirement/(\d+))",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_links_for_requirement(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });

    svr.Post(R"(/api/projects/(\d+)/tracelinks/integrity_check)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        try {
            handle_integrity_check(req, res);
        } catch (const std::exception& e) {
            json_error(res, e.what(), 500);
        }
    });
}
