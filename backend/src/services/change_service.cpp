/*
Модуль: change_service.cpp
Назначение: Реализация сервиса управления изменениями
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-11, LLR-05
*/

#include "change_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "notification_service.hpp"

ChangeRequest create_change_request_svc(int req_id, int user_id,
    const std::string& justification,
    const std::string& changes_desc, int assigned_to)
{
    auto req = Database::instance().find_requirement_by_id(req_id);
    if (!req.has_value()) {
        throw ChangeError("ERR_NOT_FOUND",
            "Требование не найдено");
    }
    if (justification.empty() || changes_desc.empty()) {
        throw ChangeError("ERR_MISSING_FIELDS",
            "Обоснование и описание обязательны");
    }
    auto cr = Database::instance().create_change_request(
        req_id, user_id, justification, changes_desc, assigned_to);
    if (assigned_to > 0) {
        send_notification(assigned_to, "change_request_created",
            "Новый запрос на изменение для " + req->system_id,
            "change_request", cr.id);
    }
    log_action(user_id, "create", "change_request",
        std::to_string(cr.id));
    return cr;
}

ChangeRequest approve_change_request_svc(int cr_id, int user_id,
    const std::string& comment)
{
    auto opt = Database::instance().find_change_request_by_id(cr_id);
    if (!opt.has_value()) {
        throw ChangeError("ERR_NOT_FOUND", "Запрос не найден");
    }
    if (opt->status != "pending") {
        throw ChangeError("ERR_ALREADY_RESOLVED",
            "Запрос уже обработан");
    }
    Database::instance().update_change_request(
        cr_id, "approved", comment, user_id);
    log_action(user_id, "approve", "change_request",
        std::to_string(cr_id));
    send_notification(opt->requested_by,
        "change_request_approved",
        "Запрос #" + std::to_string(cr_id) + " одобрен",
        "change_request", cr_id);
    return Database::instance()
        .find_change_request_by_id(cr_id).value();
}

ChangeRequest reject_change_request_svc(int cr_id, int user_id,
    const std::string& comment)
{
    auto opt = Database::instance().find_change_request_by_id(cr_id);
    if (!opt.has_value()) {
        throw ChangeError("ERR_NOT_FOUND", "Запрос не найден");
    }
    if (opt->status != "pending") {
        throw ChangeError("ERR_ALREADY_RESOLVED",
            "Запрос уже обработан");
    }
    Database::instance().update_change_request(
        cr_id, "rejected", comment, user_id);
    log_action(user_id, "reject", "change_request",
        std::to_string(cr_id));
    send_notification(opt->requested_by,
        "change_request_rejected",
        "Запрос #" + std::to_string(cr_id) + " отклонён",
        "change_request", cr_id);
    return Database::instance()
        .find_change_request_by_id(cr_id).value();
}
