/*
Модуль: audit_service.cpp
Назначение: Реализация сервиса аудит-логирования
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.Audit.Logging, LLR_SecurityManager_Audit_01
*/

#include "audit_service.hpp"
#include "database.hpp"

void log_action(int user_id, const std::string& action,
    const std::string& object_type, const std::string& object_id,
    const std::string& old_value, const std::string& new_value,
    const std::string& context)
{
    Database::instance().create_audit_log(
        user_id, action, object_type, object_id,
        old_value, new_value, context, "");
}
