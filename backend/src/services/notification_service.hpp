/*
Модуль: notification_service.hpp
Назначение: Интерфейс сервиса уведомлений
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-14, LLR-05.04
*/

#ifndef NOTIFICATION_SERVICE_HPP
#define NOTIFICATION_SERVICE_HPP

#include <string>

void send_notification(int user_id, const std::string& event_type,
    const std::string& message,
    const std::string& obj_type = "", int obj_id = 0);

#endif
