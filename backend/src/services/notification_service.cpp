/*
Модуль: notification_service.cpp
Назначение: Реализация сервиса уведомлений
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-14, LLR-05.04
*/

#include "notification_service.hpp"
#include "database.hpp"

void send_notification(int user_id, const std::string& event_type,
    const std::string& message, const std::string& obj_type,
    int obj_id)
{
    try {
        Database::instance().create_notification(
            user_id, event_type, message, obj_type, obj_id);
    } catch (...) {
        /* Уведомления не должны блокировать основную операцию */
    }
}
