/*
Модуль: auth_api.hpp
Назначение: REST API аутентификации
Автор: Разработчик
Дата создания: 21.03.2026
*/

#ifndef AUTH_API_HPP
#define AUTH_API_HPP

#include <httplib.h>

void register_auth_routes(httplib::Server& svr);

#endif
