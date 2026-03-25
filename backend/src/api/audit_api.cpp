/*
Модуль: audit_api.cpp
Назначение: Реализация REST API журнала аудита
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.Audit.Logging
*/

#include "audit_api.hpp"
#include "middleware.hpp"

void register_audit_routes(httplib::Server& svr)
{
    svr.Get("/api/audit", [](const httplib::Request& req,
        httplib::Response& res)
    {
        auto user = require_auth(req, res);
        if (!user) return;
        int page = get_int_param(req, "page", 1);
        int per_page = get_int_param(req, "per_page", 50);
        std::string obj_type = get_str_param(req, "object_type");
        int uid = get_int_param(req, "user_id", 0);

        auto result = Database::instance().get_audit_logs(
            page, per_page, obj_type, uid);
        json items = json::array();
        for (const auto& log : result.items) {
            items.push_back({
                {"id", log.id},
                {"user", log.username.empty()
                    ? json(nullptr) : json(log.username)},
                {"action", log.action},
                {"object_type", log.object_type},
                {"object_id", log.object_id.empty()
                    ? json(nullptr) : json(log.object_id)},
                {"old_value", log.old_value.empty()
                    ? json(nullptr) : json(log.old_value)},
                {"new_value", log.new_value.empty()
                    ? json(nullptr) : json(log.new_value)},
                {"context", log.context.empty()
                    ? json(nullptr) : json(log.context)},
                {"ip_address", log.ip_address.empty()
                    ? json(nullptr) : json(log.ip_address)},
                {"created_at", log.created_at}
            });
        }
        json_response(res, {
            {"items", items}, {"total", result.total},
            {"pages", result.pages}, {"page", result.page}
        });
    });
}
