/*
Модуль: requirement_service.hpp
Назначение: Интерфейс сервиса управления требованиями
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-01-FR-04, FR-07, FR-08
*/

#ifndef REQUIREMENT_SERVICE_HPP
#define REQUIREMENT_SERVICE_HPP

#include "models.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

class RequirementError : public std::runtime_error {
public:
    std::string code;
    std::string field;
    RequirementError(const std::string& c, const std::string& msg,
        const std::string& f = "")
        : std::runtime_error(msg), code(c), field(f) {}
};

Requirement create_requirement(int project_id,
    const nlohmann::json& data, int user_id);
Requirement edit_requirement(int req_id,
    const nlohmann::json& data, int user_id);
void soft_delete_requirement(int req_id, int user_id);
void hard_delete_requirement(int req_id, int user_id);
Requirement restore_requirement(int req_id, int user_id);

#endif
