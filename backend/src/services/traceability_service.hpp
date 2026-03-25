/*
Модуль: traceability_service.hpp
Назначение: Интерфейс сервиса трассировки
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-05, FR-06, FR-12, FR-16
*/

#ifndef TRACEABILITY_SERVICE_HPP
#define TRACEABILITY_SERVICE_HPP

#include "models.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

class TraceLinkError : public std::runtime_error {
public:
    std::string code;
    TraceLinkError(const std::string& c, const std::string& msg)
        : std::runtime_error(msg), code(c) {}
};

TraceLink create_trace_link(int source_id, int target_id,
    const std::string& link_type, int user_id,
    const std::string& description = "");
void delete_trace_link_svc(int link_id, int user_id);
void check_integrity(int req_id, int user_id);
std::vector<nlohmann::json> run_full_integrity_check(
    int project_id, int user_id);

#endif
