/*
Модуль: traceability_service.cpp
Назначение: Реализация сервиса управления связями трассировки
Автор: Разработчик
Дата создания: 21.03.2026
Требования: FR-05, FR-06, FR-12, LLR_TraceLinkService_CreateLink_01-02,
             LLR_TraceLinkService_IntegrityCheck_01-03
*/

#include "traceability_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "notification_service.hpp"

TraceLink create_trace_link(int source_id, int target_id,
    const std::string& link_type, int user_id, const std::string& description)
{
    if (source_id == target_id) {
        throw TraceLinkError("ERR_SELF_LINK",
            "Нельзя создать связь требования с самим собой");
    }

    auto source = Database::instance().find_requirement_by_id(source_id);
    if (!source.has_value() || source->is_deleted) {
        throw TraceLinkError("ERR_REQUIREMENT_NOT_FOUND",
            "Исходное требование не найдено");
    }

    auto target = Database::instance().find_requirement_by_id(target_id);
    if (!target.has_value() || target->is_deleted) {
        throw TraceLinkError("ERR_REQUIREMENT_NOT_FOUND",
            "Целевое требование не найдено");
    }

    if (!is_valid(link_type, VALID_LINK_TYPES)) {
        throw TraceLinkError("ERR_INVALID_LINK_TYPE",
            "Недопустимый тип связи: " + link_type);
    }

    if (Database::instance().check_trace_link_exists(
            source_id, target_id, link_type)) {
        throw TraceLinkError("ERR_DUPLICATE_LINK",
            "Связь с такими параметрами уже существует");
    }

    auto link = Database::instance().create_trace_link(
        source_id, target_id, link_type, description, user_id);

    log_action(user_id, "create", "trace_link",
        std::to_string(link.id), "",
        source->system_id + " -> " + target->system_id);
    return link;
}

void delete_trace_link_svc(int link_id, int user_id)
{
    auto link = Database::instance().find_trace_link_by_id(link_id);
    if (!link.has_value()) {
        throw TraceLinkError("ERR_LINK_NOT_FOUND", "Связь не найдена");
    }

    log_action(user_id, "delete", "trace_link",
        std::to_string(link_id));
    Database::instance().delete_trace_link(link_id);
}

void check_integrity(int req_id, int user_id)
{
    auto links = Database::instance().get_links_for_requirement(req_id);
    for (const auto& link : links) {
        auto src = Database::instance().find_requirement_by_id(link.source_req_id);
        auto tgt = Database::instance().find_requirement_by_id(link.target_req_id);

        bool is_broken = false;
        std::string reason;

        if (!src.has_value() || src->status == "deleted") {
            is_broken = true;
            reason = "Исходное требование удалено";
        }
        if (!tgt.has_value() || tgt->status == "deleted") {
            is_broken = true;
            reason = "Целевое требование удалено";
        }

        if (is_broken) {
            Database::instance().update_trace_link_status(
                link.id, "NEEDS_REVIEW");
            int resp = 0;
            if (src.has_value() && src->responsible_user_id > 0) {
                resp = src->responsible_user_id;
            } else if (tgt.has_value() && tgt->responsible_user_id > 0) {
                resp = tgt->responsible_user_id;
            }
            if (resp > 0) {
                notify_integrity_failure(
                    link.id, req_id, resp, reason);
            }
        }
    }
}

std::vector<IntegrityIssue> run_full_integrity_check(
    int project_id, int user_id)
{
    auto links = Database::instance().get_links_for_project(project_id);
    std::vector<IntegrityIssue> issues;

    for (const auto& link : links) {
        auto src = Database::instance().find_requirement_by_id(link.source_req_id);
        auto tgt = Database::instance().find_requirement_by_id(link.target_req_id);

        bool is_broken = false;
        std::string reason;

        if (!src.has_value() || src->is_deleted) {
            is_broken = true;
            reason = "Исходное требование удалено";
        } else if (!tgt.has_value() || tgt->is_deleted) {
            is_broken = true;
            reason = "Целевое требование удалено";
        }

        if (is_broken && link.status != "NEEDS_REVIEW") {
            Database::instance().update_trace_link_status(
                link.id, "NEEDS_REVIEW");
            issues.push_back({link.id, reason});
        }
    }

    log_action(user_id, "full_integrity_check", "project",
        std::to_string(project_id), "",
        "Проверено: " + std::to_string(links.size()) +
        ", проблем: " + std::to_string(issues.size()));
    return issues;
}
