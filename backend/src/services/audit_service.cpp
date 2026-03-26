/*
Модуль: audit_service.cpp
Назначение: Реализация сервиса аудита
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-17, LLR-06.01
*/

#include "audit_service.hpp"
#include "database.hpp"

void log_action(int user_id, const std::string& action,
    const std::string& obj_type, const std::string& obj_id,
    const std::string& old_val, const std::string& new_val,
    const std::string& context, const std::string& ip)
{
    try {
        Database::instance().create_audit_log(
            user_id, action, obj_type, obj_id,
            old_val, new_val, context, ip);
    } catch (...) {
        /* Аудит не должен блокировать основную операцию */
    }
}
