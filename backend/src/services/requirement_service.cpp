/*
Модуль: requirement_service.cpp
Назначение: Реализация сервиса управления требованиями
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-01-FR-04, FR-07, FR-08, LLR-01
*/

#include "requirement_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "password_utils.hpp"

using json = nlohmann::json;

Requirement create_requirement(int project_id,
    const json& data, int user_id)
{
    std::string title = data.value("title", "");
    std::string text = data.value("text", "");
    std::string category = data.value("category", "");
    std::string priority = data.value("priority", "");
    std::string status = data.value("status", "draft");

    if (title.empty() || text.empty()) {
        throw RequirementError("ERR_MISSING_FIELDS",
            "Заголовок и текст обязательны", "title");
    }
    if (!is_valid(category, VALID_CATEGORIES)) {
        throw RequirementError("ERR_INVALID_CATEGORY",
            "Недопустимая категория", "category");
    }
    if (!is_valid(priority, VALID_PRIORITIES)) {
        throw RequirementError("ERR_INVALID_PRIORITY",
            "Недопустимый приоритет", "priority");
    }

    std::string custom_id = data.value("custom_id", "");
    if (!custom_id.empty()) {
        if (Database::instance().check_custom_id_exists(
                project_id, custom_id, 0)) {
            throw RequirementError("ERR_DUPLICATE_ID",
                "Такой ID уже существует", "custom_id");
        }
    }

    Requirement req;
    req.system_id = generate_req_id();
    req.custom_id = custom_id;
    req.project_id = project_id;
    req.title = title;
    req.text = text;
    req.category = category;
    req.priority = priority;
    req.status = status;
    req.created_by = user_id;
    req.version = 1;

    if (data.contains("parent_id") && !data["parent_id"].is_null()) {
        req.parent_id = data["parent_id"].get<int>();
    }
    if (data.contains("responsible_user_id")
        && !data["responsible_user_id"].is_null()) {
        req.responsible_user_id =
            data["responsible_user_id"].get<int>();
    }

    auto created = Database::instance().create_requirement(req);
    Database::instance().add_requirement_history(
        created.id, user_id, "create", "", "", title);
    log_action(user_id, "create", "requirement",
        std::to_string(created.id), "", title);
    return created;
}

Requirement edit_requirement(int req_id,
    const json& data, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value()) {
        throw RequirementError("ERR_NOT_FOUND",
            "Требование не найдено");
    }
    Requirement req = opt.value();
    bool changed = false;

    auto track = [&](const std::string& field,
        std::string& target, const std::string& new_val)
    {
        if (!new_val.empty() && new_val != target) {
            Database::instance().add_requirement_history(
                req_id, user_id, "edit", field, target, new_val);
            target = new_val;
            changed = true;
        }
    };

    track("title", req.title, data.value("title", ""));
    track("text", req.text, data.value("text", ""));
    track("category", req.category, data.value("category", ""));
    track("priority", req.priority, data.value("priority", ""));
    track("status", req.status, data.value("status", ""));
    track("custom_id", req.custom_id, data.value("custom_id", ""));

    if (data.contains("parent_id")) {
        int new_pid = data["parent_id"].is_null()
            ? 0 : data["parent_id"].get<int>();
        if (new_pid != req.parent_id) {
            req.parent_id = new_pid;
            changed = true;
        }
    }
    if (data.contains("responsible_user_id")) {
        int new_ruid = data["responsible_user_id"].is_null()
            ? 0 : data["responsible_user_id"].get<int>();
        if (new_ruid != req.responsible_user_id) {
            req.responsible_user_id = new_ruid;
            changed = true;
        }
    }

    if (changed) {
        req.version += 1;
        req.updated_by = user_id;
        Database::instance().update_requirement(req);
        log_action(user_id, "edit", "requirement",
            std::to_string(req_id));
    }

    return Database::instance()
        .find_requirement_by_id(req_id).value();
}

void soft_delete_requirement(int req_id, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value()) {
        throw RequirementError("ERR_NOT_FOUND",
            "Требование не найдено");
    }
    Requirement req = opt.value();
    req.is_deleted = true;
    req.status = "deprecated";
    req.updated_by = user_id;
    Database::instance().update_requirement(req);
    Database::instance().add_requirement_history(
        req_id, user_id, "delete", "", "", "soft_delete");
    log_action(user_id, "delete", "requirement",
        std::to_string(req_id));
}

void hard_delete_requirement(int req_id, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value()) {
        throw RequirementError("ERR_NOT_FOUND",
            "Требование не найдено");
    }
    auto user = Database::instance().find_user_by_id(user_id);
    if (!user.has_value() || !user->can_delete()) {
        throw RequirementError("ERR_FORBIDDEN",
            "Недостаточно прав");
    }
    Database::instance().delete_requirement_hard(req_id);
    log_action(user_id, "hard_delete", "requirement",
        std::to_string(req_id));
}

Requirement restore_requirement(int req_id, int user_id)
{
    auto opt = Database::instance().find_requirement_by_id(req_id);
    if (!opt.has_value()) {
        throw RequirementError("ERR_NOT_FOUND",
            "Требование не найдено");
    }
    if (!opt->is_deleted) {
        throw RequirementError("ERR_NOT_DELETED",
            "Требование не удалено");
    }
    Requirement req = opt.value();
    req.is_deleted = false;
    req.status = "draft";
    req.updated_by = user_id;
    Database::instance().update_requirement(req);
    Database::instance().add_requirement_history(
        req_id, user_id, "restore", "", "", "");
    log_action(user_id, "restore", "requirement",
        std::to_string(req_id));
    return Database::instance()
        .find_requirement_by_id(req_id).value();
}
