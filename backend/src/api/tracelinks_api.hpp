/*
Модуль: tracelinks_api.hpp
Назначение: REST API связей трассировки между требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: Interface.Software.TraceLinks, FR-05, FR-06
*/

#ifndef TRACELINKS_API_HPP
#define TRACELINKS_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов связей трассировки
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: GET/POST/DELETE /api/projects/:pid/tracelinks, проверка целостности
*/
void register_tracelink_routes(httplib::Server& svr);

#endif
