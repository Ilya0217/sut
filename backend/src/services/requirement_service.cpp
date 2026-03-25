/*
Модуль: requirement_service.cpp
Назначение: Реализация сервиса управления требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: FR-01-FR-04, LLR_RequirementService_Create_01-05,
             LLR_RequirementService_Edit_01-02, LLR_RequirementService_Delete_01-02
*/

#include "requirement_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "notification_service.hpp"
#include "password_utils.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

/*
Назначение: Валидация обязательных полей требования
Входные данные: JSON данные
Выходные данные: исключение при ошибке
*/
static void validate_required_fields(const json& data)
{
    for (const char* field : {"title", "text", "category", "priority", "status"}) {
        std::string val = data.value(field, "");
        if (val.empty()) {
            throw RequirementError("ERR_MISSING_REQUIRED_FIELD",
                std::string("Поле '") + field + "' обязательно", field);
        }
    }
}

/*
Назначение: Валидация допустимых значений полей
Входные данные: category, priority, status
Выходные данные: исключение при ошибке
*/
static void validate_enum_fields(const json& data)
{
    std::string cat = data.value("category", "");
    if (!is_valid(cat, VALID_CATEGORIES)) {
        throw RequirementError("ERR_INVALID_CATEGORY",
            "Недопустимая категория: " + cat, "category");
    }
    std::string pri = data.value("priority", "");
    if (!is_valid(pri, VALID_PRIORITIES)) {
        throw RequirementError("ERR_INVALID_PRIORITY",
            "Недопустимый приоритет: " + pri, "priority");
    }
    std::string st = data.value("status", "");
    if (!is_valid(st, VALID_STATUSES)) {
        throw RequirementError("ERR_INVALID_STATUS",
            "Недопустимый статус: " + st, "status");
    }
}

Requirement create_requirement(int project_id, const json& data, int user_id)
{
    validate_required_fields(data);
    validate_enum_fields(data);

    std::string custom_id = data.value("custom_id", "");
    if (!custom_id.empty()) {
        if (Database::instance().check_custom_id_exists(project_id, custom_id, 0)) {
            throw RequirementError("ERR_DUPLICATE_CUSTOM_ID",
                "ID '" + custom_id + "' уже используется");
        }
    }

    Requirement req;
    req.system_id = generate_req_id();
    req.custom_id = custom_id;
    req.project_id = project_id;
    req.title = data.value("title", "");
    req.text = data.value("text", "");
    req.category = data.value("category", "");
    req.priority = data.value("priority", "");
    req.status = data.value("status", "draft");
    req.parent_id = data.value("parent_id", 0);
    req.responsible_user_id = data.value("responsible_user_id", 0);
    req.created_by = user_id;

    auto created = Database::instance().create_requirement(req);

    Database::instance().add_requirement_history(
        created.id, user_id, "CREATED", "",
        "", "Требование создано: " + created.title);

    log_action(user_id, "create", "requirement",
        created.system_id, "", created.title);
    return created;
}

Requirement edit_requirement(int req_id, const json& data, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value()) {
        throw RequirementError("ERR_REQUIREMENT_NOT_FOUND",
            "Требование не найдено");
    }
    auto req = opt.value();
    if (req.is_deleted) {
        throw RequirementError("ERR_REQUIREMENT_DELETED",
            "Требование удалено");
    }

    bool changed = false;
    auto check_field = [&](const char* field, std::string& target) {
        if (!data.contains(field)) return;
        std::string new_val = data[field].get<std::string>();
        if (new_val != target) {
            Database::instance().add_requirement_history(
                req_id, user_id, "MODIFIED", field, target, new_val);
            target = new_val;
            changed = true;
        }
    };

    check_field("title", req.title);
    check_field("text", req.text);
    check_field("category", req.category);
    check_field("priority", req.priority);
    check_field("status", req.status);
    check_field("custom_id", req.custom_id);

    if (data.contains("parent_id")) {
        req.parent_id = data.value("parent_id", 0);
        changed = true;
    }
    if (data.contains("responsible_user_id")) {
        int new_resp = data.value("responsible_user_id", 0);
        if (new_resp != req.responsible_user_id) {
            req.responsible_user_id = new_resp;
            changed = true;
            if (new_resp > 0) {
                send_notification(new_resp, "RESPONSIBILITY_ASSIGNED",
                    "Вы назначены ответственным за " + req.system_id,
                    "requirement", req_id);
            }
        }
    }

    if (changed) {
        req.version += 1;
        req.updated_by = user_id;
        Database::instance().update_requirement(req);
        log_action(user_id, "edit", "requirement", req.system_id);
    }
    return req;
}

Requirement soft_delete_requirement(int req_id, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value() || opt->is_deleted) {
        throw RequirementError("ERR_REQUIREMENT_NOT_FOUND",
            "Требование не найдено");
    }
    auto req = opt.value();
    req.is_deleted = true;
    req.status = "deleted";
    req.deleted_at = "NOW()";
    req.updated_by = user_id;
    Database::instance().update_requirement(req);

    Database::instance().add_requirement_history(
        req_id, user_id, "DELETED", "status", "active", "deleted");

    log_action(user_id, "soft_delete", "requirement", req.system_id);
    return req;
}

void hard_delete_requirement(int req_id, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value()) {
        throw RequirementError("ERR_REQUIREMENT_NOT_FOUND",
            "Требование не найдено");
    }

    auto acting_user = Database::instance().find_user_by_id(user_id);
    bool is_responsible = (opt->responsible_user_id == user_id);
    bool is_admin = acting_user.has_value() && acting_user->role == "admin";

    if (!is_responsible && !is_admin) {
        throw RequirementError("ERR_FORBIDDEN",
            "Безвозвратное удаление разрешено только ответственному "
            "лицу или администратору");
    }

    log_action(user_id, "hard_delete", "requirement", opt->system_id);
    Database::instance().delete_requirement_hard(req_id);
}

Requirement restore_requirement(int req_id, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value() || !opt->is_deleted) {
        throw RequirementError("ERR_REQUIREMENT_NOT_FOUND",
            "Удалённое требование не найдено");
    }
    auto req = opt.value();
    req.is_deleted = false;
    req.deleted_at = "";
    req.status = "draft";
    req.updated_by = user_id;
    Database::instance().update_requirement(req);

    log_action(user_id, "restore", "requirement", req.system_id);
    return req;
}
