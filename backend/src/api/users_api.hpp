/*
Модуль: users_api.hpp
Назначение: REST API управления пользователями
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: Functional.Security.UserManagement
*/

#ifndef USERS_API_HPP
#define USERS_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов управления пользователями
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: GET /api/users, PUT /api/users/:id/role
*/
void register_user_routes(httplib::Server& svr);

#endif
