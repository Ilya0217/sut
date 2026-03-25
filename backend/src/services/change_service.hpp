/*
Модуль: change_service.hpp
Назначение: Сервис управления изменениями и базовыми версиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: FR-11, LLR_BaselineManager_CreateBaseline_01,
             LLR_BaselineManager_ProcessChange_01
*/

#ifndef CHANGE_SERVICE_HPP
#define CHANGE_SERVICE_HPP

#include "models.hpp"

#include <stdexcept>
#include <string>
#include <vector>

class ChangeError : public std::runtime_error {
public:
    std::string code;
    ChangeError(const std::string& c, const std::string& msg)
        : std::runtime_error(msg), code(c) {}
};

Baseline create_baseline_svc(int project_id, const std::string& name,
    const std::vector<int>& req_ids, int user_id,
    const std::string& description);
ChangeRequest create_change_request_svc(int req_id, int user_id,
    const std::string& justification, const std::string& changes_desc,
    int assigned_to);
ChangeRequest approve_change_request_svc(int cr_id, int user_id,
    const std::string& comment);
ChangeRequest reject_change_request_svc(int cr_id, int user_id,
    const std::string& comment);

#endif
