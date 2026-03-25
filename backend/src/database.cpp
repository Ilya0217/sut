/*
Модуль: database.cpp
Назначение: Реализация слоя доступа к данным PostgreSQL
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Database.PostgreSQL, NFR-07, NFR-09
*/

#include "database.hpp"

#include <algorithm>
#include <stdexcept>

Database& Database::instance()
{
    static Database db;
    return db;
}

void Database::initialize(const std::string& connection_string)
{
    conn_ = std::make_unique<pqxx::connection>(connection_string);
}

pqxx::connection& Database::connection()
{
    return *conn_;
}

void Database::run_migrations()
{
    pqxx::work txn(*conn_);
    txn.exec(R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            username VARCHAR(80) UNIQUE NOT NULL,
            email VARCHAR(120) UNIQUE NOT NULL,
            password_hash VARCHAR(256) NOT NULL,
            role VARCHAR(20) NOT NULL DEFAULT 'analyst',
            is_active_user BOOLEAN DEFAULT TRUE,
            created_at TIMESTAMP DEFAULT NOW()
        );
        CREATE TABLE IF NOT EXISTS projects (
            id SERIAL PRIMARY KEY,
            name VARCHAR(200) NOT NULL,
            description TEXT DEFAULT '',
            created_at TIMESTAMP DEFAULT NOW(),
            created_by INTEGER REFERENCES users(id) NOT NULL
        );
        CREATE TABLE IF NOT EXISTS baselines (
            id SERIAL PRIMARY KEY,
            name VARCHAR(200) NOT NULL,
            project_id INTEGER REFERENCES projects(id) NOT NULL,
            description TEXT DEFAULT '',
            created_at TIMESTAMP DEFAULT NOW(),
            created_by INTEGER REFERENCES users(id) NOT NULL
        );
        CREATE TABLE IF NOT EXISTS requirements (
            id SERIAL PRIMARY KEY,
            system_id VARCHAR(20) UNIQUE NOT NULL,
            custom_id VARCHAR(50),
            project_id INTEGER REFERENCES projects(id) NOT NULL,
            title VARCHAR(300) NOT NULL,
            text TEXT NOT NULL,
            category VARCHAR(100) NOT NULL,
            priority VARCHAR(50) NOT NULL,
            status VARCHAR(50) NOT NULL DEFAULT 'draft',
            parent_id INTEGER REFERENCES requirements(id),
            responsible_user_id INTEGER REFERENCES users(id),
            version INTEGER DEFAULT 1,
            is_baseline BOOLEAN DEFAULT FALSE,
            baseline_id INTEGER REFERENCES baselines(id),
            created_at TIMESTAMP DEFAULT NOW(),
            updated_at TIMESTAMP DEFAULT NOW(),
            created_by INTEGER REFERENCES users(id) NOT NULL,
            updated_by INTEGER REFERENCES users(id),
            is_deleted BOOLEAN DEFAULT FALSE,
            deleted_at TIMESTAMP,
            UNIQUE(custom_id, project_id)
        );
        CREATE TABLE IF NOT EXISTS requirement_history (
            id SERIAL PRIMARY KEY,
            requirement_id INTEGER REFERENCES requirements(id) NOT NULL,
            user_id INTEGER REFERENCES users(id) NOT NULL,
            event_type VARCHAR(50) NOT NULL,
            attribute_name VARCHAR(100),
            old_value TEXT,
            new_value TEXT,
            created_at TIMESTAMP DEFAULT NOW()
        );
        CREATE TABLE IF NOT EXISTS trace_links (
            id SERIAL PRIMARY KEY,
            source_req_id INTEGER REFERENCES requirements(id) NOT NULL,
            target_req_id INTEGER REFERENCES requirements(id) NOT NULL,
            link_type VARCHAR(50) NOT NULL DEFAULT 'derives_from',
            description TEXT DEFAULT '',
            status VARCHAR(50) DEFAULT 'active',
            created_at TIMESTAMP DEFAULT NOW(),
            created_by INTEGER REFERENCES users(id) NOT NULL,
            UNIQUE(source_req_id, target_req_id, link_type)
        );
        CREATE TABLE IF NOT EXISTS change_requests (
            id SERIAL PRIMARY KEY,
            requirement_id INTEGER REFERENCES requirements(id) NOT NULL,
            requested_by INTEGER REFERENCES users(id) NOT NULL,
            assigned_to INTEGER REFERENCES users(id),
            status VARCHAR(50) DEFAULT 'pending',
            justification TEXT NOT NULL,
            changes_description TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT NOW(),
            resolved_at TIMESTAMP,
            resolution_comment TEXT
        );
        CREATE TABLE IF NOT EXISTS notifications (
            id SERIAL PRIMARY KEY,
            user_id INTEGER REFERENCES users(id) NOT NULL,
            event_type VARCHAR(100) NOT NULL,
            message TEXT NOT NULL,
            is_read BOOLEAN DEFAULT FALSE,
            related_object_type VARCHAR(50),
            related_object_id INTEGER,
            created_at TIMESTAMP DEFAULT NOW()
        );
        CREATE TABLE IF NOT EXISTS audit_log (
            id SERIAL PRIMARY KEY,
            user_id INTEGER REFERENCES users(id),
            action VARCHAR(100) NOT NULL,
            object_type VARCHAR(50) NOT NULL,
            object_id VARCHAR(50),
            old_value TEXT,
            new_value TEXT,
            context VARCHAR(200),
            ip_address VARCHAR(45),
            created_at TIMESTAMP DEFAULT NOW()
        );
    )SQL");
    txn.commit();
}

/* ===== Пользователи ===== */

std::optional<User> Database::find_user_by_id(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT id,username,email,password_hash,role,is_active_user,"
        "created_at::text FROM users WHERE id=$1", id);
    txn.commit();
    if (r.empty()) return std::nullopt;
    User u;
    u.id = r[0][0].as<int>();
    u.username = r[0][1].as<std::string>();
    u.email = r[0][2].as<std::string>();
    u.password_hash = r[0][3].as<std::string>();
    u.role = r[0][4].as<std::string>();
    u.is_active_user = r[0][5].as<bool>();
    u.created_at = r[0][6].as<std::string>();
    return u;
}

std::optional<User> Database::find_user_by_username(const std::string& username)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT id,username,email,password_hash,role,is_active_user,"
        "created_at::text FROM users WHERE username=$1", username);
    txn.commit();
    if (r.empty()) return std::nullopt;
    User u;
    u.id = r[0][0].as<int>();
    u.username = r[0][1].as<std::string>();
    u.email = r[0][2].as<std::string>();
    u.password_hash = r[0][3].as<std::string>();
    u.role = r[0][4].as<std::string>();
    u.is_active_user = r[0][5].as<bool>();
    u.created_at = r[0][6].as<std::string>();
    return u;
}

User Database::create_user(const std::string& username, const std::string& email,
                           const std::string& password_hash, const std::string& role)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO users(username,email,password_hash,role) "
        "VALUES($1,$2,$3,$4) RETURNING id,created_at::text",
        username, email, password_hash, role);
    txn.commit();
    User u;
    u.id = r[0][0].as<int>();
    u.username = username;
    u.email = email;
    u.password_hash = password_hash;
    u.role = role;
    u.created_at = r[0][1].as<std::string>();
    return u;
}

std::vector<User> Database::get_all_users()
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT id,username,email,role,created_at::text FROM users ORDER BY id");
    txn.commit();
    std::vector<User> result;
    for (const auto& r : rows) {
        User u;
        u.id = r[0].as<int>();
        u.username = r[1].as<std::string>();
        u.email = r[2].as<std::string>();
        u.role = r[3].as<std::string>();
        u.created_at = r[4].as<std::string>();
        result.push_back(u);
    }
    return result;
}

void Database::update_user_role(int user_id, const std::string& role)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params("UPDATE users SET role=$1 WHERE id=$2", role, user_id);
    txn.commit();
}

void Database::update_user_password(int user_id, const std::string& password_hash)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE users SET password_hash=$1 WHERE id=$2",
        password_hash, user_id);
    txn.commit();
}

/* ===== Проекты ===== */

Project Database::create_project(const std::string& name,
    const std::string& desc, int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO projects(name,description,created_by) "
        "VALUES($1,$2,$3) RETURNING id,created_at::text",
        name, desc, user_id);
    txn.commit();
    Project p;
    p.id = r[0][0].as<int>();
    p.name = name;
    p.description = desc;
    p.created_by = user_id;
    p.created_at = r[0][1].as<std::string>();
    return p;
}

std::optional<Project> Database::find_project_by_id(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT id,name,description,created_at::text,created_by "
        "FROM projects WHERE id=$1", id);
    txn.commit();
    if (r.empty()) return std::nullopt;
    Project p;
    p.id = r[0][0].as<int>();
    p.name = r[0][1].as<std::string>();
    p.description = r[0][2].as<std::string>();
    p.created_at = r[0][3].as<std::string>();
    p.created_by = r[0][4].as<int>();
    return p;
}

std::vector<Project> Database::get_all_projects()
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT id,name,description,created_at::text,created_by "
        "FROM projects ORDER BY created_at DESC");
    txn.commit();
    std::vector<Project> result;
    for (const auto& r : rows) {
        Project p;
        p.id = r[0].as<int>();
        p.name = r[1].as<std::string>();
        p.description = r[2].as<std::string>();
        p.created_at = r[3].as<std::string>();
        p.created_by = r[4].as<int>();
        result.push_back(p);
    }
    return result;
}

/* ===== Требования ===== */

Requirement Database::row_to_requirement(const pqxx::row& r)
{
    Requirement req;
    req.id = r["id"].as<int>();
    req.system_id = r["system_id"].as<std::string>();
    req.custom_id = r["custom_id"].is_null() ? "" : r["custom_id"].as<std::string>();
    req.project_id = r["project_id"].as<int>();
    req.title = r["title"].as<std::string>();
    req.text = r["text"].as<std::string>();
    req.category = r["category"].as<std::string>();
    req.priority = r["priority"].as<std::string>();
    req.status = r["status"].as<std::string>();
    req.parent_id = r["parent_id"].is_null() ? 0 : r["parent_id"].as<int>();
    req.responsible_user_id = r["responsible_user_id"].is_null()
        ? 0 : r["responsible_user_id"].as<int>();
    req.version = r["version"].as<int>();
    req.is_baseline = r["is_baseline"].as<bool>();
    req.created_at = r["created_at"].as<std::string>();
    req.updated_at = r["updated_at"].as<std::string>();
    req.created_by = r["created_by"].as<int>();
    req.is_deleted = r["is_deleted"].as<bool>();
    req.responsible_username = r["resp_name"].is_null()
        ? "" : r["resp_name"].as<std::string>();
    req.creator_username = r["creator_name"].is_null()
        ? "" : r["creator_name"].as<std::string>();
    return req;
}

static const char* REQ_SELECT =
    "SELECT r.*, "
    "u1.username AS resp_name, u2.username AS creator_name "
    "FROM requirements r "
    "LEFT JOIN users u1 ON r.responsible_user_id=u1.id "
    "LEFT JOIN users u2 ON r.created_by=u2.id ";

Requirement Database::create_requirement(const Requirement& req)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    std::string cid_val = req.custom_id.empty() ? "" : req.custom_id;
    auto r = txn.exec_params(
        "INSERT INTO requirements(system_id,custom_id,project_id,title,text,"
        "category,priority,status,parent_id,responsible_user_id,created_by,"
        "updated_by,version) VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,1) "
        "RETURNING id,created_at::text,updated_at::text",
        req.system_id,
        req.custom_id.empty() ? std::optional<std::string>(std::nullopt)
                              : std::optional<std::string>(req.custom_id),
        req.project_id, req.title, req.text, req.category, req.priority,
        req.status,
        req.parent_id > 0 ? std::optional<int>(req.parent_id) : std::nullopt,
        req.responsible_user_id > 0
            ? std::optional<int>(req.responsible_user_id) : std::nullopt,
        req.created_by, req.created_by);
    int new_id = r[0][0].as<int>();
    txn.commit();

    auto found = find_requirement_by_id(new_id);
    return found.value();
}

std::optional<Requirement> Database::find_requirement_by_id(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    std::string sql = std::string(REQ_SELECT) + "WHERE r.id=$1";
    auto rows = txn.exec_params(sql, id);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    return row_to_requirement(rows[0]);
}

void Database::update_requirement(const Requirement& req)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE requirements SET title=$1,text=$2,category=$3,priority=$4,"
        "status=$5,custom_id=$6,parent_id=$7,responsible_user_id=$8,"
        "version=$9,is_baseline=$10,updated_at=NOW(),updated_by=$11,"
        "is_deleted=$12,deleted_at=$13 WHERE id=$14",
        req.title, req.text, req.category, req.priority, req.status,
        req.custom_id.empty() ? std::optional<std::string>(std::nullopt)
                              : std::optional<std::string>(req.custom_id),
        req.parent_id > 0 ? std::optional<int>(req.parent_id) : std::nullopt,
        req.responsible_user_id > 0
            ? std::optional<int>(req.responsible_user_id) : std::nullopt,
        req.version, req.is_baseline, req.updated_by, req.is_deleted,
        req.deleted_at.empty() ? std::optional<std::string>(std::nullopt)
                               : std::optional<std::string>(req.deleted_at),
        req.id);
    txn.commit();
}

void Database::delete_requirement_hard(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params("DELETE FROM trace_links WHERE source_req_id=$1 OR target_req_id=$1", id);
    txn.exec_params("DELETE FROM change_requests WHERE requirement_id=$1", id);
    txn.exec_params("DELETE FROM requirement_history WHERE requirement_id=$1", id);
    txn.exec_params("DELETE FROM requirements WHERE id=$1", id);
    txn.commit();
}

Paginated<Requirement> Database::list_requirements(
    int project_id, const std::string& status, const std::string& priority,
    const std::string& category, const std::string& sort_by,
    const std::string& sort_order, int page, int per_page,
    bool include_deleted)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE r.project_id=" + txn.quote(project_id);
    if (!include_deleted) where += " AND r.is_deleted=FALSE";
    if (!status.empty()) where += " AND r.status=" + txn.quote(status);
    if (!priority.empty()) where += " AND r.priority=" + txn.quote(priority);
    if (!category.empty()) where += " AND r.category=" + txn.quote(category);

    auto cnt = txn.exec(
        "SELECT COUNT(*) FROM requirements r " + where);
    int total = cnt[0][0].as<int>();

    std::string valid_sort = "r.updated_at";
    if (sort_by == "created_at") valid_sort = "r.created_at";
    else if (sort_by == "priority") valid_sort = "r.priority";
    else if (sort_by == "title") valid_sort = "r.title";

    std::string order = (sort_order == "asc") ? "ASC" : "DESC";
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        std::string(REQ_SELECT) + where +
        " ORDER BY " + valid_sort + " " + order +
        " LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<Requirement> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total + per_page - 1) / per_page;
    for (const auto& r : rows) {
        result.items.push_back(row_to_requirement(r));
    }
    return result;
}

Paginated<Requirement> Database::search_requirements(
    int project_id, const std::string& query, int page, int per_page)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    std::string pattern = "%" + query + "%";

    auto cnt = txn.exec_params(
        "SELECT COUNT(*) FROM requirements WHERE project_id=$1 "
        "AND is_deleted=FALSE AND (system_id ILIKE $2 OR custom_id ILIKE $2 "
        "OR title ILIKE $2 OR text ILIKE $2)",
        project_id, pattern);
    int total = cnt[0][0].as<int>();

    int offset = (page - 1) * per_page;
    auto rows = txn.exec_params(
        std::string(REQ_SELECT) +
        "WHERE r.project_id=$1 AND r.is_deleted=FALSE "
        "AND (r.system_id ILIKE $2 OR r.custom_id ILIKE $2 "
        "OR r.title ILIKE $2 OR r.text ILIKE $2) "
        "ORDER BY r.updated_at DESC LIMIT $3 OFFSET $4",
        project_id, pattern, per_page, offset);
    txn.commit();

    Paginated<Requirement> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total + per_page - 1) / per_page;
    for (const auto& r : rows) {
        result.items.push_back(row_to_requirement(r));
    }
    return result;
}

bool Database::check_custom_id_exists(int project_id,
    const std::string& custom_id, int exclude_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT COUNT(*) FROM requirements "
        "WHERE project_id=$1 AND custom_id=$2 AND id!=$3",
        project_id, custom_id, exclude_id);
    txn.commit();
    return r[0][0].as<int>() > 0;
}

/* ===== История требований ===== */

void Database::add_requirement_history(int req_id, int user_id,
    const std::string& event_type, const std::string& attr_name,
    const std::string& old_val, const std::string& new_val)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "INSERT INTO requirement_history(requirement_id,user_id,event_type,"
        "attribute_name,old_value,new_value) VALUES($1,$2,$3,$4,$5,$6)",
        req_id, user_id, event_type,
        attr_name.empty() ? std::optional<std::string>(std::nullopt)
                          : std::optional<std::string>(attr_name),
        old_val, new_val);
    txn.commit();
}

Paginated<RequirementHistory> Database::get_requirement_history(
    int req_id, int page, int per_page)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto cnt = txn.exec_params(
        "SELECT COUNT(*) FROM requirement_history WHERE requirement_id=$1",
        req_id);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;
    auto rows = txn.exec_params(
        "SELECT h.*,h.created_at::text AS cat,u.username FROM requirement_history h "
        "LEFT JOIN users u ON h.user_id=u.id "
        "WHERE h.requirement_id=$1 ORDER BY h.created_at DESC "
        "LIMIT $2 OFFSET $3",
        req_id, per_page, offset);
    txn.commit();

    Paginated<RequirementHistory> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total + per_page - 1) / per_page;
    for (const auto& r : rows) {
        RequirementHistory h;
        h.id = r["id"].as<int>();
        h.requirement_id = r["requirement_id"].as<int>();
        h.user_id = r["user_id"].as<int>();
        h.event_type = r["event_type"].as<std::string>();
        h.attribute_name = r["attribute_name"].is_null()
            ? "" : r["attribute_name"].as<std::string>();
        h.old_value = r["old_value"].is_null()
            ? "" : r["old_value"].as<std::string>();
        h.new_value = r["new_value"].is_null()
            ? "" : r["new_value"].as<std::string>();
        h.created_at = r["cat"].as<std::string>();
        h.username = r["username"].is_null()
            ? "" : r["username"].as<std::string>();
        result.items.push_back(h);
    }
    return result;
}

void Database::delete_requirement_history(int req_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "DELETE FROM requirement_history WHERE requirement_id=$1", req_id);
    txn.commit();
}

/* ===== Связи трассировки ===== */

TraceLink Database::row_to_trace_link(const pqxx::row& r)
{
    TraceLink lnk;
    lnk.id = r["id"].as<int>();
    lnk.source_req_id = r["source_req_id"].as<int>();
    lnk.target_req_id = r["target_req_id"].as<int>();
    lnk.link_type = r["link_type"].as<std::string>();
    lnk.description = r["description"].is_null()
        ? "" : r["description"].as<std::string>();
    lnk.status = r["status"].as<std::string>();
    lnk.created_at = r["link_created"].as<std::string>();
    lnk.created_by = r["created_by"].as<int>();
    lnk.source_system_id = r["src_sid"].is_null()
        ? "" : r["src_sid"].as<std::string>();
    lnk.source_title = r["src_title"].is_null()
        ? "" : r["src_title"].as<std::string>();
    lnk.target_system_id = r["tgt_sid"].is_null()
        ? "" : r["tgt_sid"].as<std::string>();
    lnk.target_title = r["tgt_title"].is_null()
        ? "" : r["tgt_title"].as<std::string>();
    lnk.creator_username = r["creator_name"].is_null()
        ? "" : r["creator_name"].as<std::string>();
    return lnk;
}

static const char* LINK_SELECT =
    "SELECT tl.*, tl.created_at::text AS link_created, "
    "rs.system_id AS src_sid, rs.title AS src_title, "
    "rt.system_id AS tgt_sid, rt.title AS tgt_title, "
    "u.username AS creator_name "
    "FROM trace_links tl "
    "LEFT JOIN requirements rs ON tl.source_req_id=rs.id "
    "LEFT JOIN requirements rt ON tl.target_req_id=rt.id "
    "LEFT JOIN users u ON tl.created_by=u.id ";

TraceLink Database::create_trace_link(int source_id, int target_id,
    const std::string& link_type, const std::string& description, int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO trace_links(source_req_id,target_req_id,link_type,"
        "description,status,created_by) VALUES($1,$2,$3,$4,'active',$5) "
        "RETURNING id",
        source_id, target_id, link_type, description, user_id);
    int new_id = r[0][0].as<int>();
    auto rows = txn.exec_params(
        std::string(LINK_SELECT) + "WHERE tl.id=$1", new_id);
    txn.commit();
    return row_to_trace_link(rows[0]);
}

std::optional<TraceLink> Database::find_trace_link_by_id(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        std::string(LINK_SELECT) + "WHERE tl.id=$1", id);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    return row_to_trace_link(rows[0]);
}

bool Database::check_trace_link_exists(int source_id, int target_id,
                                       const std::string& link_type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT COUNT(*) FROM trace_links "
        "WHERE source_req_id=$1 AND target_req_id=$2 AND link_type=$3",
        source_id, target_id, link_type);
    txn.commit();
    return r[0][0].as<int>() > 0;
}

std::vector<TraceLink> Database::get_links_for_project(int project_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        std::string(LINK_SELECT) +
        "WHERE rs.project_id=$1 ORDER BY tl.created_at DESC", project_id);
    txn.commit();
    std::vector<TraceLink> result;
    for (const auto& r : rows) result.push_back(row_to_trace_link(r));
    return result;
}

std::vector<TraceLink> Database::get_links_for_requirement(int req_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        std::string(LINK_SELECT) +
        "WHERE tl.source_req_id=$1 OR tl.target_req_id=$1", req_id);
    txn.commit();
    std::vector<TraceLink> result;
    for (const auto& r : rows) result.push_back(row_to_trace_link(r));
    return result;
}

void Database::delete_trace_link(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params("DELETE FROM trace_links WHERE id=$1", id);
    txn.commit();
}

void Database::update_trace_link_status(int id, const std::string& status)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params("UPDATE trace_links SET status=$1 WHERE id=$2", status, id);
    txn.commit();
}

/* ===== Базовые версии ===== */

Baseline Database::create_baseline(int project_id, const std::string& name,
    const std::string& description, int user_id,
    const std::vector<int>& req_ids)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO baselines(project_id,name,description,created_by) "
        "VALUES($1,$2,$3,$4) RETURNING id,created_at::text",
        project_id, name, description, user_id);
    int bl_id = r[0][0].as<int>();
    for (int rid : req_ids) {
        txn.exec_params(
            "UPDATE requirements SET is_baseline=TRUE,baseline_id=$1 WHERE id=$2",
            bl_id, rid);
    }
    txn.commit();

    Baseline bl;
    bl.id = bl_id;
    bl.name = name;
    bl.project_id = project_id;
    bl.description = description;
    bl.created_by = user_id;
    bl.created_at = r[0][1].as<std::string>();
    bl.requirements_count = static_cast<int>(req_ids.size());
    return bl;
}

std::vector<Baseline> Database::get_baselines(int project_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        "SELECT b.id,b.name,b.description,b.created_at::text,b.created_by,"
        "(SELECT COUNT(*) FROM requirements WHERE baseline_id=b.id) AS cnt "
        "FROM baselines b WHERE b.project_id=$1 ORDER BY b.created_at DESC",
        project_id);
    txn.commit();
    std::vector<Baseline> result;
    for (const auto& r : rows) {
        Baseline bl;
        bl.id = r[0].as<int>();
        bl.name = r[1].as<std::string>();
        bl.description = r[2].as<std::string>();
        bl.created_at = r[3].as<std::string>();
        bl.created_by = r[4].as<int>();
        bl.requirements_count = r[5].as<int>();
        result.push_back(bl);
    }
    return result;
}

/* ===== Запросы на изменение ===== */

ChangeRequest Database::create_change_request(int req_id, int user_id,
    const std::string& justification, const std::string& changes_desc,
    int assigned_to)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO change_requests(requirement_id,requested_by,assigned_to,"
        "justification,changes_description) VALUES($1,$2,$3,$4,$5) "
        "RETURNING id,created_at::text",
        req_id, user_id,
        assigned_to > 0 ? std::optional<int>(assigned_to) : std::nullopt,
        justification, changes_desc);
    txn.commit();

    ChangeRequest cr;
    cr.id = r[0][0].as<int>();
    cr.requirement_id = req_id;
    cr.requested_by = user_id;
    cr.assigned_to = assigned_to;
    cr.status = "pending";
    cr.justification = justification;
    cr.changes_description = changes_desc;
    cr.created_at = r[0][1].as<std::string>();
    return cr;
}

Paginated<ChangeRequest> Database::get_change_requests(int project_id,
    const std::string& status, int page, int per_page)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE r.project_id=" + txn.quote(project_id);
    if (!status.empty()) {
        where += " AND cr.status=" + txn.quote(status);
    }

    auto cnt = txn.exec(
        "SELECT COUNT(*) FROM change_requests cr "
        "JOIN requirements r ON cr.requirement_id=r.id " + where);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        "SELECT cr.*,cr.created_at::text AS cat,cr.resolved_at::text AS rat,"
        "r.system_id AS req_sid,u1.username AS req_by,u2.username AS asn_to "
        "FROM change_requests cr "
        "JOIN requirements r ON cr.requirement_id=r.id "
        "LEFT JOIN users u1 ON cr.requested_by=u1.id "
        "LEFT JOIN users u2 ON cr.assigned_to=u2.id " + where +
        " ORDER BY cr.created_at DESC LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<ChangeRequest> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total + per_page - 1) / per_page;
    for (const auto& r : rows) {
        ChangeRequest cr;
        cr.id = r["id"].as<int>();
        cr.requirement_id = r["requirement_id"].as<int>();
        cr.status = r["status"].as<std::string>();
        cr.justification = r["justification"].as<std::string>();
        cr.changes_description = r["changes_description"].as<std::string>();
        cr.created_at = r["cat"].is_null() ? "" : r["cat"].as<std::string>();
        cr.resolved_at = r["rat"].is_null() ? "" : r["rat"].as<std::string>();
        cr.resolution_comment = r["resolution_comment"].is_null()
            ? "" : r["resolution_comment"].as<std::string>();
        cr.requirement_system_id = r["req_sid"].as<std::string>();
        cr.requester_username = r["req_by"].is_null()
            ? "" : r["req_by"].as<std::string>();
        cr.assignee_username = r["asn_to"].is_null()
            ? "" : r["asn_to"].as<std::string>();
        result.items.push_back(cr);
    }
    return result;
}

std::optional<ChangeRequest> Database::find_change_request_by_id(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        "SELECT cr.*,cr.created_at::text AS cat,cr.resolved_at::text AS rat,"
        "r.system_id AS req_sid,u1.username AS req_by,u2.username AS asn_to "
        "FROM change_requests cr "
        "JOIN requirements r ON cr.requirement_id=r.id "
        "LEFT JOIN users u1 ON cr.requested_by=u1.id "
        "LEFT JOIN users u2 ON cr.assigned_to=u2.id WHERE cr.id=$1", id);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    ChangeRequest cr;
    cr.id = rows[0]["id"].as<int>();
    cr.requirement_id = rows[0]["requirement_id"].as<int>();
    cr.status = rows[0]["status"].as<std::string>();
    cr.justification = rows[0]["justification"].as<std::string>();
    cr.changes_description = rows[0]["changes_description"].as<std::string>();
    cr.created_at = rows[0]["cat"].is_null() ? "" : rows[0]["cat"].as<std::string>();
    cr.resolved_at = rows[0]["rat"].is_null() ? "" : rows[0]["rat"].as<std::string>();
    cr.resolution_comment = rows[0]["resolution_comment"].is_null()
        ? "" : rows[0]["resolution_comment"].as<std::string>();
    cr.requirement_system_id = rows[0]["req_sid"].as<std::string>();
    cr.requester_username = rows[0]["req_by"].is_null()
        ? "" : rows[0]["req_by"].as<std::string>();
    cr.assignee_username = rows[0]["asn_to"].is_null()
        ? "" : rows[0]["asn_to"].as<std::string>();
    return cr;
}

void Database::update_change_request(int id, const std::string& status,
    const std::string& comment, int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE change_requests SET status=$1,resolution_comment=$2,"
        "resolved_at=NOW() WHERE id=$3",
        status, comment, id);
    txn.commit();
}

/* ===== Уведомления ===== */

void Database::create_notification(int user_id, const std::string& event_type,
    const std::string& message, const std::string& obj_type, int obj_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "INSERT INTO notifications(user_id,event_type,message,"
        "related_object_type,related_object_id) VALUES($1,$2,$3,$4,$5)",
        user_id, event_type, message,
        obj_type.empty() ? std::optional<std::string>(std::nullopt)
                         : std::optional<std::string>(obj_type),
        obj_id > 0 ? std::optional<int>(obj_id) : std::nullopt);
    txn.commit();
}

Paginated<Notification> Database::get_notifications(int user_id,
    bool unread_only, int page, int per_page)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE user_id=" + txn.quote(user_id);
    if (unread_only) where += " AND is_read=FALSE";

    auto cnt = txn.exec("SELECT COUNT(*) FROM notifications " + where);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        "SELECT *,created_at::text AS cat FROM notifications " + where +
        " ORDER BY created_at DESC LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<Notification> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total + per_page - 1) / per_page;
    for (const auto& r : rows) {
        Notification n;
        n.id = r["id"].as<int>();
        n.user_id = r["user_id"].as<int>();
        n.event_type = r["event_type"].as<std::string>();
        n.message = r["message"].as<std::string>();
        n.is_read = r["is_read"].as<bool>();
        n.related_object_type = r["related_object_type"].is_null()
            ? "" : r["related_object_type"].as<std::string>();
        n.related_object_id = r["related_object_id"].is_null()
            ? 0 : r["related_object_id"].as<int>();
        n.created_at = r["cat"].as<std::string>();
        result.items.push_back(n);
    }
    return result;
}

int Database::get_unread_count(int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT COUNT(*) FROM notifications "
        "WHERE user_id=$1 AND is_read=FALSE", user_id);
    txn.commit();
    return r[0][0].as<int>();
}

void Database::mark_notification_read(int id, int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE notifications SET is_read=TRUE WHERE id=$1 AND user_id=$2",
        id, user_id);
    txn.commit();
}

/* ===== Аудит ===== */

void Database::create_audit_log(int user_id, const std::string& action,
    const std::string& obj_type, const std::string& obj_id,
    const std::string& old_val, const std::string& new_val,
    const std::string& context, const std::string& ip)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "INSERT INTO audit_log(user_id,action,object_type,object_id,"
        "old_value,new_value,context,ip_address) "
        "VALUES($1,$2,$3,$4,$5,$6,$7,$8)",
        user_id > 0 ? std::optional<int>(user_id) : std::nullopt,
        action, obj_type,
        obj_id.empty() ? std::optional<std::string>(std::nullopt)
                       : std::optional<std::string>(obj_id),
        old_val.empty() ? std::optional<std::string>(std::nullopt)
                        : std::optional<std::string>(old_val),
        new_val.empty() ? std::optional<std::string>(std::nullopt)
                        : std::optional<std::string>(new_val),
        context.empty() ? std::optional<std::string>(std::nullopt)
                        : std::optional<std::string>(context),
        ip.empty() ? std::optional<std::string>(std::nullopt)
                   : std::optional<std::string>(ip));
    txn.commit();
}

Paginated<AuditLog> Database::get_audit_logs(int page, int per_page,
    const std::string& obj_type, int user_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE 1=1";
    if (!obj_type.empty()) where += " AND a.object_type=" + txn.quote(obj_type);
    if (user_id > 0) where += " AND a.user_id=" + txn.quote(user_id);

    auto cnt = txn.exec("SELECT COUNT(*) FROM audit_log a " + where);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        "SELECT a.*,a.created_at::text AS cat,u.username "
        "FROM audit_log a LEFT JOIN users u ON a.user_id=u.id " + where +
        " ORDER BY a.created_at DESC LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<AuditLog> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total + per_page - 1) / per_page;
    for (const auto& r : rows) {
        AuditLog log;
        log.id = r["id"].as<int>();
        log.user_id = r["user_id"].is_null() ? 0 : r["user_id"].as<int>();
        log.action = r["action"].as<std::string>();
        log.object_type = r["object_type"].as<std::string>();
        log.object_id = r["object_id"].is_null()
            ? "" : r["object_id"].as<std::string>();
        log.old_value = r["old_value"].is_null()
            ? "" : r["old_value"].as<std::string>();
        log.new_value = r["new_value"].is_null()
            ? "" : r["new_value"].as<std::string>();
        log.context = r["context"].is_null()
            ? "" : r["context"].as<std::string>();
        log.ip_address = r["ip_address"].is_null()
            ? "" : r["ip_address"].as<std::string>();
        log.created_at = r["cat"].as<std::string>();
        log.username = r["username"].is_null()
            ? "" : r["username"].as<std::string>();
        result.items.push_back(log);
    }
    return result;
}
