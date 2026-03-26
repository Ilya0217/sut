/*
Модуль: notifications_api.hpp
Назначение: REST API уведомлений пользователей
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: Functional.NotificationSystem.Alerts
*/

#ifndef NOTIFICATIONS_API_HPP
#define NOTIFICATIONS_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов уведомлений
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: GET /api/notifications, POST /api/notifications/:id/read,
          GET /api/notifications/unread_count
*/
void register_notification_routes(httplib::Server& svr);

#endif
