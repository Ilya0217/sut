/*
Модуль: notification_service.cpp
Назначение: Реализация сервиса уведомлений
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.NotificationSystem.Alerts
*/

#include "notification_service.hpp"
#include "database.hpp"

void send_notification(int user_id, const std::string& event_type,
    const std::string& message, const std::string& obj_type, int obj_id)
{
    Database::instance().create_notification(
        user_id, event_type, message, obj_type, obj_id);
}

void notify_integrity_failure(int link_id, int requirement_id,
    int responsible_user_id, const std::string& reason)
{
    std::string msg = "Нарушение целостности связи #"
        + std::to_string(link_id) + " для требования #"
        + std::to_string(requirement_id) + ": " + reason;
    send_notification(responsible_user_id, "INTEGRITY_FAILURE",
        msg, "trace_link", link_id);
}
