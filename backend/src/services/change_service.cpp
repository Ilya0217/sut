/*
Модуль: change_service.cpp
Назначение: Реализация сервиса управления изменениями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: FR-11, LLR_BaselineManager_CreateBaseline_01,
             LLR_BaselineManager_ProcessChange_01
*/

#include "change_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "notification_service.hpp"

Baseline create_baseline_svc(int project_id, const std::string& name,
    const std::vector<int>& req_ids, int user_id,
    const std::string& description)
{
    if (name.empty()) {
        throw ChangeError("ERR_MISSING_NAME", "Название обязательно");
    }
    if (req_ids.empty()) {
        throw ChangeError("ERR_NO_REQUIREMENTS",
            "Выберите хотя бы одно требование");
    }
    auto bl = Database::instance().create_baseline(
        project_id, name, description, user_id, req_ids);
    log_action(user_id, "create", "baseline",
        std::to_string(bl.id), "", name);
    return bl;
}

ChangeRequest create_change_request_svc(int req_id, int user_id,
    const std::string& justification, const std::string& changes_desc,
    int assigned_to)
{
    auto req = Database::instance().find_requirement_by_id(req_id);
    if (!req.has_value()) {
        throw ChangeError("ERR_REQUIREMENT_NOT_FOUND",
            "Требование не найдено");
    }

    auto cr = Database::instance().create_change_request(
        req_id, user_id, justification, changes_desc, assigned_to);

    if (assigned_to > 0) {
        send_notification(assigned_to, "CHANGE_REQUEST_ASSIGNED",
            "Вам назначен запрос на изменение #" + std::to_string(cr.id),
            "change_request", cr.id);
    }

    log_action(user_id, "create", "change_request",
        std::to_string(cr.id));
    cr.requirement_system_id = req->system_id;
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
        throw ChangeError("ERR_INVALID_STATUS",
            "Запрос уже обработан");
    }

    Database::instance().update_change_request(
        cr_id, "approved", comment, user_id);
    log_action(user_id, "approve", "change_request",
        std::to_string(cr_id));

    auto result = Database::instance().find_change_request_by_id(cr_id);
    return result.value();
}

ChangeRequest reject_change_request_svc(int cr_id, int user_id,
    const std::string& comment)
{
    auto opt = Database::instance().find_change_request_by_id(cr_id);
    if (!opt.has_value()) {
        throw ChangeError("ERR_NOT_FOUND", "Запрос не найден");
    }
    if (opt->status != "pending") {
        throw ChangeError("ERR_INVALID_STATUS",
            "Запрос уже обработан");
    }

    Database::instance().update_change_request(
        cr_id, "rejected", comment, user_id);
    log_action(user_id, "reject", "change_request",
        std::to_string(cr_id));

    auto result = Database::instance().find_change_request_by_id(cr_id);
    return result.value();
}
