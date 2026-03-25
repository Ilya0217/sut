/*
Модуль: import_export_service.cpp
Назначение: Реализация импорта и экспорта данных
Автор: Разработчик
Дата создания: 21.03.2026
Требования: FR-09, FR-10, Functional.ImportExport
*/

#include "import_export_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "password_utils.hpp"

#include <sstream>

using json = nlohmann::json;

/*
Назначение: Экранирование значения для CSV
Входные данные: строка
Выходные данные: экранированная строка
*/
static std::string escape_csv(const std::string& val)
{
    if (val.find(',') != std::string::npos ||
        val.find('"') != std::string::npos ||
        val.find('\n') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : val) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }
    return val;
}

std::string export_to_json(int project_id, int user_id)
{
    auto page = Database::instance().list_requirements(
        project_id, "", "", "", "created_at", "asc", 1, 10000, false);

    json arr = json::array();
    for (const auto& r : page.items) {
        arr.push_back({
            {"system_id", r.system_id}, {"custom_id", r.custom_id},
            {"title", r.title}, {"text", r.text},
            {"category", r.category}, {"priority", r.priority},
            {"status", r.status}, {"version", r.version}
        });
    }

    log_action(user_id, "export_json", "project",
        std::to_string(project_id), "",
        std::to_string(page.items.size()) + " требований");
    return arr.dump(2);
}

std::string export_to_csv(int project_id, int user_id)
{
    auto page = Database::instance().list_requirements(
        project_id, "", "", "", "created_at", "asc", 1, 10000, false);

    std::ostringstream oss;
    oss << "system_id,custom_id,title,text,category,priority,status,version\n";
    for (const auto& r : page.items) {
        oss << escape_csv(r.system_id) << ","
            << escape_csv(r.custom_id) << ","
            << escape_csv(r.title) << ","
            << escape_csv(r.text) << ","
            << escape_csv(r.category) << ","
            << escape_csv(r.priority) << ","
            << escape_csv(r.status) << ","
            << r.version << "\n";
    }

    log_action(user_id, "export_csv", "project",
        std::to_string(project_id));
    return oss.str();
}

json import_from_json(const std::string& content,
    int project_id, int user_id)
{
    json data;
    try {
        data = json::parse(content);
    } catch (...) {
        throw ImportExportError("ERR_INVALID_JSON",
            "Некорректный формат JSON");
    }

    if (!data.is_array()) {
        throw ImportExportError("ERR_INVALID_FORMAT",
            "Ожидается массив объектов");
    }

    int imported = 0;
    for (const auto& item : data) {
        Requirement req;
        req.system_id = generate_req_id();
        req.project_id = project_id;
        req.title = item.value("title", "");
        req.text = item.value("text", "");
        req.category = item.value("category", "functional");
        req.priority = item.value("priority", "medium");
        req.status = item.value("status", "draft");
        req.custom_id = item.value("custom_id", "");
        req.created_by = user_id;

        if (req.title.empty() || req.text.empty()) continue;
        if (!is_valid(req.category, VALID_CATEGORIES)) continue;
        if (!is_valid(req.priority, VALID_PRIORITIES)) continue;

        Database::instance().create_requirement(req);
        ++imported;
    }

    log_action(user_id, "import_json", "project",
        std::to_string(project_id), "",
        std::to_string(imported) + " требований");
    return {{"imported_requirements", imported}};
}

/*
Назначение: Разбор строки CSV
Входные данные: строка CSV
Выходные данные: вектор полей
*/
static std::vector<std::string> parse_csv_line(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

json import_from_csv(const std::string& content,
    int project_id, int user_id)
{
    std::istringstream stream(content);
    std::string line;
    std::getline(stream, line);

    int imported = 0;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        auto fields = parse_csv_line(line);
        if (fields.size() < 5) continue;

        Requirement req;
        req.system_id = generate_req_id();
        req.project_id = project_id;
        req.title = fields.size() > 2 ? fields[2] : "";
        req.text = fields.size() > 3 ? fields[3] : "";
        req.category = fields.size() > 4 ? fields[4] : "functional";
        req.priority = fields.size() > 5 ? fields[5] : "medium";
        req.status = fields.size() > 6 ? fields[6] : "draft";
        req.custom_id = fields.size() > 1 ? fields[1] : "";
        req.created_by = user_id;

        if (req.title.empty() || req.text.empty()) continue;
        Database::instance().create_requirement(req);
        ++imported;
    }

    log_action(user_id, "import_csv", "project",
        std::to_string(project_id));
    return {{"imported_requirements", imported}};
}
