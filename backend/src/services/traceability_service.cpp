/*
Модуль: traceability_service.cpp
Назначение: Реализация сервиса трассировки
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-05, FR-06, FR-12, FR-16, LLR-02
*/

#include "traceability_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"

using json = nlohmann::json;

TraceLink create_trace_link(int source_id, int target_id,
    const std::string& link_type, int user_id,
    const std::string& description)
{
    if (source_id == target_id) {
        throw TraceLinkError("ERR_SELF_LINK",
            "Нельзя создать связь на само себя");
    }
    auto src = Database::instance().find_requirement_by_id(source_id);
    if (!src.has_value()) {
        throw TraceLinkError("ERR_SOURCE_NOT_FOUND",
            "Исходное требование не найдено");
    }
    auto tgt = Database::instance().find_requirement_by_id(target_id);
    if (!tgt.has_value()) {
        throw TraceLinkError("ERR_TARGET_NOT_FOUND",
            "Целевое требование не найдено");
    }
    if (!is_valid(link_type, VALID_LINK_TYPES)) {
        throw TraceLinkError("ERR_INVALID_TYPE",
            "Недопустимый тип связи");
    }
    if (Database::instance().check_trace_link_exists(
            source_id, target_id, link_type)) {
        throw TraceLinkError("ERR_DUPLICATE",
            "Такая связь уже существует");
    }

    auto link = Database::instance().create_trace_link(
        source_id, target_id, link_type, description, user_id);
    log_action(user_id, "create", "trace_link",
        std::to_string(link.id));
    return link;
}

void delete_trace_link_svc(int link_id, int user_id)
{
    auto opt = Database::instance().find_trace_link_by_id(link_id);
    if (!opt.has_value()) {
        throw TraceLinkError("ERR_NOT_FOUND", "Связь не найдена");
    }
    Database::instance().delete_trace_link(link_id);
    log_action(user_id, "delete", "trace_link",
        std::to_string(link_id));
}

void check_integrity(int req_id, int user_id)
{
    auto links = Database::instance()
        .get_links_for_requirement(req_id);
    for (const auto& link : links) {
        bool is_broken = false;
        auto src = Database::instance()
            .find_requirement_by_id(link.source_req_id);
        auto tgt = Database::instance()
            .find_requirement_by_id(link.target_req_id);
        if (!src.has_value() || src->is_deleted) is_broken = true;
        if (!tgt.has_value() || tgt->is_deleted) is_broken = true;
        std::string new_st = is_broken ? "NEEDS_REVIEW" : "active";
        if (new_st != link.status) {
            Database::instance().update_trace_link_status(
                link.id, new_st);
        }
    }
}

std::vector<json> run_full_integrity_check(int project_id,
    int user_id)
{
    auto links = Database::instance()
        .get_links_for_project(project_id);
    std::vector<json> issues;
    for (const auto& link : links) {
        bool is_broken = false;
        std::string reason;
        auto src = Database::instance()
            .find_requirement_by_id(link.source_req_id);
        auto tgt = Database::instance()
            .find_requirement_by_id(link.target_req_id);
        if (!src.has_value() || src->is_deleted) {
            is_broken = true;
            reason = "Исходное требование удалено";
        }
        if (!tgt.has_value() || tgt->is_deleted) {
            is_broken = true;
            reason = "Целевое требование удалено";
        }
        std::string new_st = is_broken ? "NEEDS_REVIEW" : "active";
        if (new_st != link.status) {
            Database::instance().update_trace_link_status(
                link.id, new_st);
        }
        if (is_broken) {
            issues.push_back({
                {"link_id", link.id}, {"reason", reason}});
        }
    }
    log_action(user_id, "integrity_check", "trace_link",
        "", "", std::to_string(issues.size()) + " issues");
    return issues;
}
