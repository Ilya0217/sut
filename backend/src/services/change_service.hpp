/*
Модуль: change_service.hpp
Назначение: Интерфейс сервиса управления изменениями
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-11, LLR-05
*/

#ifndef CHANGE_SERVICE_HPP
#define CHANGE_SERVICE_HPP

#include "models.hpp"
#include <stdexcept>
#include <string>

class ChangeError : public std::runtime_error {
public:
    std::string code;
    ChangeError(const std::string& c, const std::string& msg)
        : std::runtime_error(msg), code(c) {}
};

ChangeRequest create_change_request_svc(int req_id, int user_id,
    const std::string& justification,
    const std::string& changes_desc, int assigned_to);
ChangeRequest approve_change_request_svc(int cr_id, int user_id,
    const std::string& comment);
ChangeRequest reject_change_request_svc(int cr_id, int user_id,
    const std::string& comment);

#endif
