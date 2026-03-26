/*
Модуль: projects_api.hpp
Назначение: REST API управления проектами
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: Interface.Software.Projects
*/

#ifndef PROJECTS_API_HPP
#define PROJECTS_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов управления проектами
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: GET /api/projects, POST /api/projects, GET /api/projects/:id
*/
void register_project_routes(httplib::Server& svr);

#endif
