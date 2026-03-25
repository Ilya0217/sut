/*
Модуль: notification_service.hpp
Назначение: Сервис уведомлений
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.NotificationSystem.Alerts
*/

#ifndef NOTIFICATION_SERVICE_HPP
#define NOTIFICATION_SERVICE_HPP

#include <string>

void send_notification(int user_id, const std::string& event_type,
    const std::string& message, const std::string& obj_type = "",
    int obj_id = 0);

void notify_integrity_failure(int link_id, int requirement_id,
    int responsible_user_id, const std::string& reason);

#endif
