/*
Модуль: notifications_api.cpp
Назначение: Реализация REST API уведомлений
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.NotificationSystem.Alerts
*/

#include "notifications_api.hpp"
#include "middleware.hpp"

void register_notification_routes(httplib::Server& svr)
{
    svr.Get("/api/notifications", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int page = get_int_param(req, "page", 1);
        bool unread = get_str_param(req, "unread") == "1";
        auto result = Database::instance().get_notifications(
            user->id, unread, page, 20);
        json items = json::array();
        for (const auto& n : result.items) {
            items.push_back({
                {"id", n.id}, {"event_type", n.event_type},
                {"message", n.message}, {"is_read", n.is_read},
                {"related_object_type", n.related_object_type},
                {"related_object_id", n.related_object_id},
                {"created_at", n.created_at}
            });
        }
        int unread_count = Database::instance().get_unread_count(user->id);
        json_response(res, {
            {"items", items}, {"total", result.total},
            {"pages", result.pages}, {"page", result.page},
            {"unread_count", unread_count}
        });
    });

    svr.Post(R"(/api/notifications/(\d+)/read)",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int nid = std::stoi(req.matches[1]);
        Database::instance().mark_notification_read(nid, user->id);
        json_response(res, {{"message", "OK"}});
    });

    svr.Get("/api/notifications/unread_count",
        [](const httplib::Request& req, httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int count = Database::instance().get_unread_count(user->id);
        json_response(res, {{"count", count}});
    });
}
