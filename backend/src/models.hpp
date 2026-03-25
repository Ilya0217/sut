/*
Модуль: models.hpp
Назначение: Модели данных системы управления требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Database.PostgreSQL
*/

#ifndef MODELS_HPP
#define MODELS_HPP

#include <string>
#include <vector>

struct User {
    int id = 0;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string role;
    bool is_active_user = true;
    std::string created_at;

    bool can_create() const { return role == "analyst" || role == "admin"; }
    bool can_edit() const { return role == "analyst" || role == "admin"; }
    bool can_delete() const { return role == "analyst" || role == "admin"; }
    bool can_manage_links() const { return role == "analyst" || role == "admin"; }
    bool can_approve_changes() const { return role == "reviewer" || role == "admin"; }
    bool can_manage_users() const { return role == "admin"; }
    bool can_import_export() const { return role == "analyst" || role == "admin"; }
    bool can_run_integrity_check() const { return role == "analyst" || role == "admin"; }
};

struct Project {
    int id = 0;
    std::string name;
    std::string description;
    std::string created_at;
    int created_by = 0;
};

struct Requirement {
    int id = 0;
    std::string system_id;
    std::string custom_id;
    int project_id = 0;
    std::string title;
    std::string text;
    std::string category;
    std::string priority;
    std::string status;
    int parent_id = 0;
    int responsible_user_id = 0;
    int version = 1;
    bool is_baseline = false;
    int baseline_id = 0;
    std::string created_at;
    std::string updated_at;
    int created_by = 0;
    int updated_by = 0;
    bool is_deleted = false;
    std::string deleted_at;

    std::string responsible_username;
    std::string creator_username;
};

const std::vector<std::string> VALID_STATUSES = {
    "draft", "active", "approved", "implemented",
    "verified", "deleted", "deprecated"
};
const std::vector<std::string> VALID_PRIORITIES = {
    "critical", "high", "medium", "low"
};
const std::vector<std::string> VALID_CATEGORIES = {
    "functional", "non_functional", "interface",
    "performance", "security", "derived"
};
const std::vector<std::string> VALID_LINK_TYPES = {
    "derives_from", "satisfies", "verifies", "implements"
};
const std::vector<std::string> VALID_ROLES = {
    "analyst", "reviewer", "admin"
};

struct RequirementHistory {
    int id = 0;
    int requirement_id = 0;
    int user_id = 0;
    std::string event_type;
    std::string attribute_name;
    std::string old_value;
    std::string new_value;
    std::string created_at;
    std::string username;
};

struct TraceLink {
    int id = 0;
    int source_req_id = 0;
    int target_req_id = 0;
    std::string link_type;
    std::string description;
    std::string status;
    std::string created_at;
    int created_by = 0;

    std::string source_system_id;
    std::string source_title;
    std::string target_system_id;
    std::string target_title;
    std::string creator_username;
};

struct ChangeRequest {
    int id = 0;
    int requirement_id = 0;
    int requested_by = 0;
    int assigned_to = 0;
    std::string status;
    std::string justification;
    std::string changes_description;
    std::string created_at;
    std::string resolved_at;
    std::string resolution_comment;

    std::string requirement_system_id;
    std::string requester_username;
    std::string assignee_username;
};

struct Baseline {
    int id = 0;
    std::string name;
    int project_id = 0;
    std::string description;
    std::string created_at;
    int created_by = 0;
    int requirements_count = 0;
};

struct Notification {
    int id = 0;
    int user_id = 0;
    std::string event_type;
    std::string message;
    bool is_read = false;
    std::string related_object_type;
    int related_object_id = 0;
    std::string created_at;
};

struct AuditLog {
    int id = 0;
    int user_id = 0;
    std::string action;
    std::string object_type;
    std::string object_id;
    std::string old_value;
    std::string new_value;
    std::string context;
    std::string ip_address;
    std::string created_at;
    std::string username;
};

template <typename T>
struct Paginated {
    std::vector<T> items;
    int total = 0;
    int pages = 0;
    int page = 1;
    int per_page = 50;
};

/*
Назначение: Проверка принадлежности строки списку допустимых значений
Входные данные: значение, вектор допустимых
Выходные данные: true если найдено
*/
inline bool is_valid(const std::string& val, const std::vector<std::string>& valid)
{
    for (const auto& v : valid) {
        if (v == val) return true;
    }
    return false;
}

#endif
