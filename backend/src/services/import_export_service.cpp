/*
Модуль: import_export_service.cpp
Назначение: Реализация сервиса импорта/экспорта
Автор: Разработчик
Дата создания: 25.03.2026
Требования: FR-09, FR-10, LLR-04
*/

#include "import_export_service.hpp"
#include "audit_service.hpp"
#include "database.hpp"
#include "password_utils.hpp"

#include <sstream>

using json = nlohmann::json;

std::string export_to_json(int project_id, int user_id)
{
    auto page = Database::instance().list_requirements(
        project_id, "", "", "", "updated_at", "desc",
        1, 10000, false);
    json arr = json::array();
    for (const auto& r : page.items) {
        arr.push_back({
            {"system_id", r.system_id},
            {"custom_id", r.custom_id},
            {"title", r.title}, {"text", r.text},
            {"category", r.category},
            {"priority", r.priority},
            {"status", r.status},
            {"version", r.version}});
    }
    log_action(user_id, "export", "requirement", "", "", "json");
    return arr.dump(2);
}

static std::string csv_escape(const std::string& s)
{
    if (s.find(',') != std::string::npos
        || s.find('"') != std::string::npos
        || s.find('\n') != std::string::npos) {
        std::string out = "\"";
        for (char c : s) {
            if (c == '"') out += "\"\"";
            else out += c;
        }
        return out + "\"";
    }
    return s;
}

std::string export_to_csv(int project_id, int user_id)
{
    auto page = Database::instance().list_requirements(
        project_id, "", "", "", "updated_at", "desc",
        1, 10000, false);
    std::string csv = "system_id,custom_id,title,text,"
        "category,priority,status,version\n";
    for (const auto& r : page.items) {
        csv += csv_escape(r.system_id) + ","
            + csv_escape(r.custom_id) + ","
            + csv_escape(r.title) + ","
            + csv_escape(r.text) + ","
            + csv_escape(r.category) + ","
            + csv_escape(r.priority) + ","
            + csv_escape(r.status) + ","
            + std::to_string(r.version) + "\n";
    }
    log_action(user_id, "export", "requirement", "", "", "csv");
    return csv;
}

static std::vector<std::string> parse_csv_line(
    const std::string& line)
{
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"' && i + 1 < line.size()
                && line[i + 1] == '"') {
                current += '"';
                ++i;
            } else if (c == '"') {
                in_quotes = false;
            } else {
                current += c;
            }
        } else if (c == '"') {
            in_quotes = true;
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

json import_from_json(const std::string& content,
    int project_id, int user_id)
{
    json data;
    try { data = json::parse(content); }
    catch (...) {
        throw ImportExportError("ERR_PARSE", "Невалидный JSON");
    }
    if (!data.is_array()) {
        throw ImportExportError("ERR_FORMAT", "Ожидается массив");
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
        req.version = 1;
        if (req.title.empty()) continue;
        Database::instance().create_requirement(req);
        ++imported;
    }
    log_action(user_id, "import", "requirement",
        "", "", std::to_string(imported) + " json");
    return {{"imported_requirements", imported}};
}

json import_from_csv(const std::string& content,
    int project_id, int user_id)
{
    std::istringstream stream(content);
    std::string line;
    int imported = 0;
    bool is_header = true;
    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r") continue;
        if (is_header) { is_header = false; continue; }
        auto fields = parse_csv_line(line);
        if (fields.size() < 7) continue;
        Requirement req;
        req.system_id = generate_req_id();
        req.project_id = project_id;
        req.custom_id = fields[1];
        req.title = fields[2];
        req.text = fields[3];
        req.category = fields[4];
        req.priority = fields[5];
        req.status = fields[6];
        req.created_by = user_id;
        req.version = 1;
        if (req.title.empty()) continue;
        Database::instance().create_requirement(req);
        ++imported;
    }
    log_action(user_id, "import", "requirement",
        "", "", std::to_string(imported) + " csv");
    return {{"imported_requirements", imported}};
}
