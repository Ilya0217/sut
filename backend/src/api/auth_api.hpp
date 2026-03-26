/*
Модуль: auth_api.hpp
Назначение: REST API аутентификации (логин, регистрация, выход, текущий пользователь)
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: LLR_SecurityManager_Authenticate_01
*/

#ifndef AUTH_API_HPP
#define AUTH_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов аутентификации
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: POST /api/auth/login, POST /api/auth/register,
          POST /api/auth/logout, GET /api/auth/me
*/
void register_auth_routes(httplib::Server& svr);

#endif
