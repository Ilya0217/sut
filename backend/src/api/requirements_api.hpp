/*
Модуль: requirements_api.hpp
Назначение: REST API управления требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: Interface.Software.Requirements, FR-01, FR-02, FR-03, FR-04
*/

#ifndef REQUIREMENTS_API_HPP
#define REQUIREMENTS_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов управления требованиями
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: CRUD /api/projects/:pid/requirements, история, удаление, восстановление
*/
void register_requirement_routes(httplib::Server& svr);

#endif
