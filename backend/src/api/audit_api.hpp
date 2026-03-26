/*
Модуль: audit_api.hpp
Назначение: REST API журнала аудита
Автор: Разработчик
Дата создания: 21.03.2026
Изменения: 26.03.2026 — добавлена обработка исключений во всех обработчиках
Требования: Functional.Audit.Logging
*/

#ifndef AUDIT_API_HPP
#define AUDIT_API_HPP

#include <httplib.h>

/*
Назначение: Регистрация маршрутов журнала аудита
Входные данные: svr — HTTP сервер
Выходные данные: нет
Маршруты: GET /api/audit
*/
void register_audit_routes(httplib::Server& svr);

#endif
