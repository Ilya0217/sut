/*
Модуль: import_export_service.hpp
Назначение: Интерфейс сервиса импорта/экспорта
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-09, FR-10
*/

#ifndef IMPORT_EXPORT_SERVICE_HPP
#define IMPORT_EXPORT_SERVICE_HPP

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

class ImportExportError : public std::runtime_error {
public:
    std::string code;
    ImportExportError(const std::string& c, const std::string& msg)
        : std::runtime_error(msg), code(c) {}
};

std::string export_to_json(int project_id, int user_id);
std::string export_to_csv(int project_id, int user_id);
nlohmann::json import_from_json(const std::string& content,
    int project_id, int user_id);
nlohmann::json import_from_csv(const std::string& content,
    int project_id, int user_id);

#endif
