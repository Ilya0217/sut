/*
Модуль: audit_service.hpp
Назначение: Сервис аудит-логирования
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.Audit.Logging, LLR_SecurityManager_Audit_01
*/

#ifndef AUDIT_SERVICE_HPP
#define AUDIT_SERVICE_HPP

#include <string>

void log_action(int user_id, const std::string& action,
    const std::string& object_type, const std::string& object_id = "",
    const std::string& old_value = "", const std::string& new_value = "",
    const std::string& context = "");

#endif
