/*
Модуль: projects_api.hpp
Назначение: REST API управления проектами
Автор: Разработчик
Дата создания: 21.03.2026
*/

#ifndef PROJECTS_API_HPP
#define PROJECTS_API_HPP

#include <httplib.h>

void register_project_routes(httplib::Server& svr);

#endif
