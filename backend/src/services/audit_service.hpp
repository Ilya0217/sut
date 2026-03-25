/*
Модуль: audit_service.hpp
Назначение: Интерфейс сервиса аудита
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-17, LLR-06.01
*/

#ifndef AUDIT_SERVICE_HPP
#define AUDIT_SERVICE_HPP

#include <string>

void log_action(int user_id, const std::string& action,
    const std::string& obj_type,
    const std::string& obj_id = "",
    const std::string& old_val = "",
    const std::string& new_val = "",
    const std::string& context = "",
    const std::string& ip = "");

#endif
