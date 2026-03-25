/*
Модуль: database.hpp
Назначение: Интерфейс слоя доступа к данным PostgreSQL
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Database.PostgreSQL, NFR-07
*/

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "models.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <pqxx/pqxx>
#include <string>

class Database {
public:
    static Database& instance();
    void initialize(const std::string& connection_string);
    void run_migrations();
    pqxx::connection& connection();

    /* Пользователи */
    std::optional<User> find_user_by_id(int id);
    std::optional<User> find_user_by_username(const std::string& username);
    User create_user(const std::string& username, const std::string& email,
                     const std::string& password_hash, const std::string& role);
    std::vector<User> get_all_users();
    void update_user_role(int user_id, const std::string& role);
    void update_user_password(int user_id, const std::string& password_hash);

    /* Проекты */
    Project create_project(const std::string& name, const std::string& desc, int user_id);
    std::optional<Project> find_project_by_id(int id);
    std::vector<Project> get_all_projects();

    /* Требования */
    Requirement create_requirement(const Requirement& req);
    std::optional<Requirement> find_requirement_by_id(int id);
    void update_requirement(const Requirement& req);
    void delete_requirement_hard(int id);
    Paginated<Requirement> list_requirements(
        int project_id, const std::string& status, const std::string& priority,
        const std::string& category, const std::string& sort_by,
        const std::string& sort_order, int page, int per_page,
        bool include_deleted);
    Paginated<Requirement> search_requirements(
        int project_id, const std::string& query, int page, int per_page);
    bool check_custom_id_exists(int project_id, const std::string& custom_id, int exclude_id);

    /* История требований */
    void add_requirement_history(int req_id, int user_id,
        const std::string& event_type, const std::string& attr_name,
        const std::string& old_val, const std::string& new_val);
    Paginated<RequirementHistory> get_requirement_history(
        int req_id, int page, int per_page);
    void delete_requirement_history(int req_id);

    /* Связи трассировки */
    TraceLink create_trace_link(int source_id, int target_id,
        const std::string& link_type, const std::string& description, int user_id);
    std::optional<TraceLink> find_trace_link_by_id(int id);
    bool check_trace_link_exists(int source_id, int target_id,
                                 const std::string& link_type);
    std::vector<TraceLink> get_links_for_project(int project_id);
    std::vector<TraceLink> get_links_for_requirement(int req_id);
    void delete_trace_link(int id);
    void update_trace_link_status(int id, const std::string& status);

    /* Базовые версии */
    Baseline create_baseline(int project_id, const std::string& name,
        const std::string& description, int user_id,
        const std::vector<int>& req_ids);
    std::vector<Baseline> get_baselines(int project_id);

    /* Запросы на изменение */
    ChangeRequest create_change_request(int req_id, int user_id,
        const std::string& justification, const std::string& changes_desc,
        int assigned_to);
    Paginated<ChangeRequest> get_change_requests(int project_id,
        const std::string& status, int page, int per_page);
    std::optional<ChangeRequest> find_change_request_by_id(int id);
    void update_change_request(int id, const std::string& status,
        const std::string& comment, int user_id);

    /* Уведомления */
    void create_notification(int user_id, const std::string& event_type,
        const std::string& message, const std::string& obj_type, int obj_id);
    Paginated<Notification> get_notifications(int user_id, bool unread_only,
        int page, int per_page);
    int get_unread_count(int user_id);
    void mark_notification_read(int id, int user_id);

    /* Аудит */
    void create_audit_log(int user_id, const std::string& action,
        const std::string& obj_type, const std::string& obj_id,
        const std::string& old_val, const std::string& new_val,
        const std::string& context, const std::string& ip);
    Paginated<AuditLog> get_audit_logs(int page, int per_page,
        const std::string& obj_type, int user_id);

private:
    Database() = default;
    std::unique_ptr<pqxx::connection> conn_;
    std::mutex mutex_;

    Requirement row_to_requirement(const pqxx::row& r);
    TraceLink row_to_trace_link(const pqxx::row& r);
};

#endif
