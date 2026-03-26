/*
Модуль: changes_api.hpp
Назначение: REST API базовых версий и запросов на изменение
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: FR-11, Functional.ConfigurationManagement.BaselineAndChange
*/

#ifndef CHANGES_API_HPP
#define CHANGES_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов базовых версий и запросов на изменение
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: GET/POST /api/projects/:pid/baselines,
          GET/POST /api/projects/:pid/change_requests,
          POST .../approve, POST .../reject
*/
void register_change_routes(httplib::Server& svr);

#endif
