/*
Модуль: database.cpp
Назначение: Реализация слоя доступа к данным PostgreSQL
Автор: Разработчик
Дата создания: 25.03.2026
Требования: Interface.Software.Database.PostgreSQL, NFR-07, NFR-09
*/

#include "database.hpp"

#include <algorithm>
#include <cmath>
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

void Database::run_migrations()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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
            deleted_at TIMESTAMP
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

    txn.exec(R"SQL(
        ALTER TABLE users
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE projects
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE requirements
            ALTER COLUMN created_at SET DEFAULT NOW(),
            ALTER COLUMN updated_at SET DEFAULT NOW();
        ALTER TABLE requirement_history
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE trace_links
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE change_requests
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE notifications
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE audit_log
            ALTER COLUMN created_at SET DEFAULT NOW();
        ALTER TABLE baselines
            ALTER COLUMN created_at SET DEFAULT NOW();
        UPDATE projects SET created_at = NOW()
            WHERE created_at IS NULL;
        UPDATE requirements SET created_at = NOW()
            WHERE created_at IS NULL;
        UPDATE requirements SET updated_at = NOW()
            WHERE updated_at IS NULL;
        UPDATE requirements SET is_deleted = false
            WHERE is_deleted IS NULL;
        UPDATE requirements SET is_baseline = false
            WHERE is_baseline IS NULL;
        ALTER TABLE requirements
            ALTER COLUMN is_deleted SET DEFAULT false;
        ALTER TABLE requirements
            ALTER COLUMN is_baseline SET DEFAULT false;
        UPDATE trace_links SET status = 'active'
            WHERE status IS NULL;
        UPDATE trace_links SET created_at = NOW()
            WHERE created_at IS NULL;
        ALTER TABLE trace_links
            ALTER COLUMN status SET DEFAULT 'active';
        UPDATE change_requests SET status = 'pending'
            WHERE status IS NULL;
        ALTER TABLE change_requests
            ALTER COLUMN status SET DEFAULT 'pending';
    )SQL");

    txn.commit();
}

/* ===== Утилита: nullable int → pqxx param ===== */

static std::optional<int> nullable_int(int val)
{
    if (val <= 0) return std::nullopt;
    return val;
}

/* ===== Утилита: safe string из row ===== */

static std::string safe_str(const pqxx::row& r, int col)
{
    if (r[col].is_null()) return "";
    return r[col].as<std::string>();
}

static int safe_int(const pqxx::row& r, int col)
{
    if (r[col].is_null()) return 0;
    return r[col].as<int>();
}

static bool safe_bool(const pqxx::row& r, int col)
{
    if (r[col].is_null()) return false;
    return r[col].as<bool>();
}

/* ===== Пользователи ===== */

static User row_to_user(const pqxx::row& r)
{
    User u;
    u.id = r[0].as<int>();
    u.username = safe_str(r, 1);
    u.email = safe_str(r, 2);
    u.password_hash = safe_str(r, 3);
    u.role = safe_str(r, 4);
    u.is_active_user = safe_bool(r, 5);
    u.created_at = safe_str(r, 6);
    return u;
}

std::optional<User> Database::find_user_by_id(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT id,username,email,password_hash,role,"
        "is_active_user,created_at::text FROM users WHERE id=$1", id);
    txn.commit();
    if (r.empty()) return std::nullopt;
    return row_to_user(r[0]);
}

std::optional<User> Database::find_user_by_username(
    const std::string& username)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT id,username,email,password_hash,role,"
        "is_active_user,created_at::text FROM users WHERE username=$1",
        username);
    txn.commit();
    if (r.empty()) return std::nullopt;
    return row_to_user(r[0]);
}

User Database::create_user(const std::string& username,
    const std::string& email, const std::string& password_hash,
    const std::string& role)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO users(username,email,password_hash,role,created_at) "
        "VALUES($1,$2,$3,$4,NOW()) "
        "RETURNING id,username,email,password_hash,role,"
        "is_active_user,created_at::text",
        username, email, password_hash, role);
    txn.commit();
    return row_to_user(r[0]);
}

std::vector<User> Database::get_all_users()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT id,username,email,'',role,is_active_user,"
        "created_at::text FROM users ORDER BY id");
    txn.commit();
    std::vector<User> result;
    for (const auto& r : rows) {
        result.push_back(row_to_user(r));
    }
    return result;
}

void Database::update_user_role(int user_id, const std::string& role)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE users SET role=$1 WHERE id=$2", role, user_id);
    txn.commit();
}

void Database::update_user_password(int user_id,
    const std::string& password_hash)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO projects(name,description,created_by,created_at) "
        "VALUES($1,$2,$3,NOW()) RETURNING id,created_at::text",
        name, desc, user_id);
    txn.commit();
    Project p;
    p.id = r[0][0].as<int>();
    p.name = name;
    p.description = desc;
    p.created_at = r[0][1].as<std::string>();
    p.created_by = user_id;
    return p;
}

std::optional<Project> Database::find_project_by_id(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT id,name,description,created_at::text,created_by "
        "FROM projects WHERE id=$1", id);
    txn.commit();
    if (r.empty()) return std::nullopt;
    Project p;
    p.id = r[0][0].as<int>();
    p.name = r[0][1].as<std::string>();
    p.description = safe_str(r[0], 2);
    p.created_at = safe_str(r[0], 3);
    p.created_by = r[0][4].as<int>();
    return p;
}

std::vector<Project> Database::get_all_projects()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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
        p.description = safe_str(r, 2);
        p.created_at = safe_str(r, 3);
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
    req.system_id = safe_str(r, r.column_number("system_id"));
    req.custom_id = safe_str(r, r.column_number("custom_id"));
    req.project_id = r["project_id"].as<int>();
    req.title = safe_str(r, r.column_number("title"));
    req.text = safe_str(r, r.column_number("text"));
    req.category = safe_str(r, r.column_number("category"));
    req.priority = safe_str(r, r.column_number("priority"));
    req.status = safe_str(r, r.column_number("status"));
    req.parent_id = safe_int(r, r.column_number("parent_id"));
    req.responsible_user_id = safe_int(r,
        r.column_number("responsible_user_id"));
    req.version = r["version"].as<int>();
    req.is_baseline = safe_bool(r, r.column_number("is_baseline"));
    req.is_deleted = safe_bool(r, r.column_number("is_deleted"));
    req.created_at = safe_str(r, r.column_number("cat"));
    req.updated_at = safe_str(r, r.column_number("uat"));
    req.created_by = safe_int(r, r.column_number("created_by"));
    req.responsible_username = safe_str(r,
        r.column_number("resp_name"));
    req.creator_username = safe_str(r,
        r.column_number("creator_name"));
    return req;
}

static const char* REQ_SELECT =
    "SELECT r.*,r.created_at::text AS cat,r.updated_at::text AS uat,"
    "COALESCE(ru.username,'') AS resp_name,"
    "COALESCE(cu.username,'') AS creator_name "
    "FROM requirements r "
    "LEFT JOIN users ru ON ru.id=r.responsible_user_id "
    "LEFT JOIN users cu ON cu.id=r.created_by ";

Requirement Database::create_requirement(const Requirement& req)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO requirements"
        "(system_id,custom_id,project_id,title,text,category,"
        "priority,status,parent_id,responsible_user_id,version,"
        "created_by,created_at,updated_at,is_deleted,is_baseline) "
        "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,NOW(),NOW(),false,false) "
        "RETURNING id",
        req.system_id,
        req.custom_id.empty() ? std::optional<std::string>{}
                              : req.custom_id,
        req.project_id, req.title, req.text, req.category,
        req.priority, req.status,
        nullable_int(req.parent_id),
        nullable_int(req.responsible_user_id),
        req.version, req.created_by);
    int new_id = r[0][0].as<int>();
    auto row = txn.exec_params(
        std::string(REQ_SELECT) + "WHERE r.id=$1", new_id);
    txn.commit();
    return row_to_requirement(row[0]);
}

std::optional<Requirement> Database::find_requirement_by_id(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        std::string(REQ_SELECT) + "WHERE r.id=$1", id);
    txn.commit();
    if (r.empty()) return std::nullopt;
    return row_to_requirement(r[0]);
}

void Database::update_requirement(const Requirement& req)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE requirements SET "
        "title=$1,text=$2,category=$3,priority=$4,status=$5,"
        "parent_id=$6,responsible_user_id=$7,version=$8,"
        "is_baseline=$9,is_deleted=$10,"
        "deleted_at=CASE WHEN $10 THEN COALESCE(deleted_at,NOW()) "
        "ELSE NULL END,"
        "updated_at=NOW(),updated_by=$11,custom_id=$12 "
        "WHERE id=$13",
        req.title, req.text, req.category, req.priority, req.status,
        nullable_int(req.parent_id),
        nullable_int(req.responsible_user_id),
        req.version, req.is_baseline, req.is_deleted,
        nullable_int(req.updated_by),
        req.custom_id.empty() ? std::optional<std::string>{}
                              : req.custom_id,
        req.id);
    txn.commit();
}

void Database::delete_requirement_hard(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "DELETE FROM requirement_history WHERE requirement_id=$1", id);
    txn.exec_params(
        "DELETE FROM trace_links "
        "WHERE source_req_id=$1 OR target_req_id=$1", id);
    txn.exec_params(
        "DELETE FROM change_requests WHERE requirement_id=$1", id);
    txn.exec_params("DELETE FROM requirements WHERE id=$1", id);
    txn.commit();
}

Paginated<Requirement> Database::list_requirements(
    int project_id, const std::string& status,
    const std::string& priority, const std::string& category,
    const std::string& sort_by, const std::string& sort_order,
    int page, int per_page, bool include_deleted)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string base_where = "WHERE r.project_id=" +
        std::to_string(project_id);
    if (!include_deleted) {
        base_where += " AND COALESCE(r.is_deleted,false)=false";
    }
    if (!status.empty()) {
        base_where += " AND r.status=" + txn.quote(status);
    }
    if (!priority.empty()) {
        base_where += " AND r.priority=" + txn.quote(priority);
    }
    if (!category.empty()) {
        base_where += " AND r.category=" + txn.quote(category);
    }

    auto cnt = txn.exec(
        "SELECT COUNT(*) FROM requirements r " + base_where);
    int total = cnt[0][0].as<int>();

    std::string allowed_sort = "updated_at";
    if (sort_by == "created_at" || sort_by == "priority"
        || sort_by == "title" || sort_by == "status") {
        allowed_sort = sort_by;
    }
    std::string order = (sort_order == "asc") ? "ASC" : "DESC";

    int offset = (page - 1) * per_page;
    auto rows = txn.exec(
        std::string(REQ_SELECT) + base_where +
        " ORDER BY r." + allowed_sort + " " + order +
        " LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<Requirement> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total > 0)
        ? static_cast<int>(std::ceil(
            static_cast<double>(total) / per_page))
        : 1;
    for (const auto& r : rows) {
        result.items.push_back(row_to_requirement(r));
    }
    return result;
}

Paginated<Requirement> Database::search_requirements(
    int project_id, const std::string& query, int page, int per_page)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string pattern = "%" + query + "%";
    auto cnt = txn.exec_params(
        "SELECT COUNT(*) FROM requirements r "
        "WHERE r.project_id=$1 AND r.is_deleted=false "
        "AND (r.title ILIKE $2 OR r.text ILIKE $2 "
        "OR r.system_id ILIKE $2 OR r.custom_id ILIKE $2)",
        project_id, pattern);
    int total = cnt[0][0].as<int>();

    int offset = (page - 1) * per_page;
    auto rows = txn.exec_params(
        std::string(REQ_SELECT) +
        "WHERE r.project_id=$1 AND r.is_deleted=false "
        "AND (r.title ILIKE $2 OR r.text ILIKE $2 "
        "OR r.system_id ILIKE $2 OR r.custom_id ILIKE $2) "
        "ORDER BY r.updated_at DESC LIMIT $3 OFFSET $4",
        project_id, pattern, per_page, offset);
    txn.commit();

    Paginated<Requirement> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total > 0)
        ? static_cast<int>(std::ceil(
            static_cast<double>(total) / per_page))
        : 1;
    for (const auto& r : rows) {
        result.items.push_back(row_to_requirement(r));
    }
    return result;
}

bool Database::check_custom_id_exists(int project_id,
    const std::string& custom_id, int exclude_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT 1 FROM requirements "
        "WHERE project_id=$1 AND custom_id=$2 AND id!=$3",
        project_id, custom_id, exclude_id);
    txn.commit();
    return !r.empty();
}

/* ===== История требований ===== */

void Database::add_requirement_history(int req_id, int user_id,
    const std::string& event_type, const std::string& attr_name,
    const std::string& old_val, const std::string& new_val)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "INSERT INTO requirement_history"
        "(requirement_id,user_id,event_type,attribute_name,"
        "old_value,new_value,created_at) VALUES($1,$2,$3,$4,$5,$6,NOW())",
        req_id, user_id, event_type,
        attr_name.empty() ? std::optional<std::string>{}
                          : attr_name,
        old_val.empty() ? std::optional<std::string>{} : old_val,
        new_val.empty() ? std::optional<std::string>{} : new_val);
    txn.commit();
}

Paginated<RequirementHistory> Database::get_requirement_history(
    int req_id, int page, int per_page)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    auto cnt = txn.exec_params(
        "SELECT COUNT(*) FROM requirement_history "
        "WHERE requirement_id=$1", req_id);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec_params(
        "SELECT h.id,h.requirement_id,h.user_id,h.event_type,"
        "h.attribute_name,h.old_value,h.new_value,"
        "h.created_at::text AS cat,COALESCE(u.username,'') AS uname "
        "FROM requirement_history h "
        "LEFT JOIN users u ON u.id=h.user_id "
        "WHERE h.requirement_id=$1 "
        "ORDER BY h.created_at DESC LIMIT $2 OFFSET $3",
        req_id, per_page, offset);
    txn.commit();

    Paginated<RequirementHistory> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total > 0)
        ? static_cast<int>(std::ceil(
            static_cast<double>(total) / per_page))
        : 1;
    for (const auto& r : rows) {
        RequirementHistory h;
        h.id = r[0].as<int>();
        h.requirement_id = r[1].as<int>();
        h.user_id = r[2].as<int>();
        h.event_type = r[3].as<std::string>();
        h.attribute_name = safe_str(r, 4);
        h.old_value = safe_str(r, 5);
        h.new_value = safe_str(r, 6);
        h.created_at = safe_str(r, 7);
        h.username = safe_str(r, 8);
        result.items.push_back(h);
    }
    return result;
}

void Database::delete_requirement_history(int req_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "DELETE FROM requirement_history WHERE requirement_id=$1",
        req_id);
    txn.commit();
}

/* ===== Связи трассировки ===== */

TraceLink Database::row_to_trace_link(const pqxx::row& r)
{
    TraceLink lnk;
    lnk.id = r["id"].as<int>();
    lnk.source_req_id = r["source_req_id"].as<int>();
    lnk.target_req_id = r["target_req_id"].as<int>();
    lnk.link_type = safe_str(r, r.column_number("link_type"));
    lnk.description = safe_str(r, r.column_number("description"));
    lnk.status = safe_str(r, r.column_number("status"));
    lnk.created_at = safe_str(r, r.column_number("lcat"));
    lnk.created_by = safe_int(r, r.column_number("created_by"));
    lnk.source_system_id = safe_str(r,
        r.column_number("src_sid"));
    lnk.source_title = safe_str(r, r.column_number("src_title"));
    lnk.target_system_id = safe_str(r,
        r.column_number("tgt_sid"));
    lnk.target_title = safe_str(r, r.column_number("tgt_title"));
    lnk.creator_username = safe_str(r,
        r.column_number("creator_name"));
    return lnk;
}

static const char* LINK_SELECT =
    "SELECT tl.*,tl.created_at::text AS lcat,"
    "COALESCE(sr.system_id,'') AS src_sid,"
    "COALESCE(sr.title,'') AS src_title,"
    "COALESCE(tr.system_id,'') AS tgt_sid,"
    "COALESCE(tr.title,'') AS tgt_title,"
    "COALESCE(cu.username,'') AS creator_name "
    "FROM trace_links tl "
    "LEFT JOIN requirements sr ON sr.id=tl.source_req_id "
    "LEFT JOIN requirements tr ON tr.id=tl.target_req_id "
    "LEFT JOIN users cu ON cu.id=tl.created_by ";

TraceLink Database::create_trace_link(int source_id, int target_id,
    const std::string& link_type, const std::string& description,
    int user_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO trace_links"
        "(source_req_id,target_req_id,link_type,description,created_by,created_at,status)"
        " VALUES($1,$2,$3,$4,$5,NOW(),'active') RETURNING id",
        source_id, target_id, link_type, description, user_id);
    int new_id = r[0][0].as<int>();
    auto row = txn.exec_params(
        std::string(LINK_SELECT) + "WHERE tl.id=$1", new_id);
    txn.commit();
    return row_to_trace_link(row[0]);
}

std::optional<TraceLink> Database::find_trace_link_by_id(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        std::string(LINK_SELECT) + "WHERE tl.id=$1", id);
    txn.commit();
    if (r.empty()) return std::nullopt;
    return row_to_trace_link(r[0]);
}

bool Database::check_trace_link_exists(int source_id, int target_id,
    const std::string& link_type)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT 1 FROM trace_links "
        "WHERE source_req_id=$1 AND target_req_id=$2 "
        "AND link_type=$3",
        source_id, target_id, link_type);
    txn.commit();
    return !r.empty();
}

std::vector<TraceLink> Database::get_links_for_project(int project_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        std::string(LINK_SELECT) +
        "WHERE sr.project_id=$1 ORDER BY tl.created_at DESC",
        project_id);
    txn.commit();
    std::vector<TraceLink> result;
    for (const auto& r : rows) {
        result.push_back(row_to_trace_link(r));
    }
    return result;
}

std::vector<TraceLink> Database::get_links_for_requirement(int req_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        std::string(LINK_SELECT) +
        "WHERE tl.source_req_id=$1 OR tl.target_req_id=$1 "
        "ORDER BY tl.created_at DESC", req_id);
    txn.commit();
    std::vector<TraceLink> result;
    for (const auto& r : rows) {
        result.push_back(row_to_trace_link(r));
    }
    return result;
}

void Database::delete_trace_link(int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params("DELETE FROM trace_links WHERE id=$1", id);
    txn.commit();
}

void Database::update_trace_link_status(int id,
    const std::string& status)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE trace_links SET status=$1 WHERE id=$2", status, id);
    txn.commit();
}

/* ===== Базовые версии ===== */

Baseline Database::create_baseline(int project_id,
    const std::string& name, const std::string& description,
    int user_id, const std::vector<int>& req_ids)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO baselines(project_id,name,description,created_by,created_at)"
        " VALUES($1,$2,$3,$4,NOW()) RETURNING id,created_at::text",
        project_id, name, description, user_id);
    int bl_id = r[0][0].as<int>();
    std::string bl_created = r[0][1].as<std::string>();

    for (int rid : req_ids) {
        txn.exec_params(
            "UPDATE requirements SET is_baseline=true,"
            "baseline_id=$1 WHERE id=$2", bl_id, rid);
    }
    txn.commit();

    Baseline bl;
    bl.id = bl_id;
    bl.name = name;
    bl.project_id = project_id;
    bl.description = description;
    bl.created_at = bl_created;
    bl.created_by = user_id;
    bl.requirements_count = static_cast<int>(req_ids.size());
    return bl;
}

std::vector<Baseline> Database::get_baselines(int project_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        "SELECT b.id,b.name,b.project_id,b.description,"
        "b.created_at::text,b.created_by,"
        "(SELECT COUNT(*) FROM requirements "
        "WHERE baseline_id=b.id) AS cnt "
        "FROM baselines b WHERE b.project_id=$1 "
        "ORDER BY b.created_at DESC", project_id);
    txn.commit();
    std::vector<Baseline> result;
    for (const auto& r : rows) {
        Baseline bl;
        bl.id = r[0].as<int>();
        bl.name = safe_str(r, 1);
        bl.project_id = r[2].as<int>();
        bl.description = safe_str(r, 3);
        bl.created_at = safe_str(r, 4);
        bl.created_by = r[5].as<int>();
        bl.requirements_count = r[6].as<int>();
        result.push_back(bl);
    }
    return result;
}

/* ===== Запросы на изменение ===== */

ChangeRequest Database::create_change_request(int req_id,
    int user_id, const std::string& justification,
    const std::string& changes_desc, int assigned_to)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "INSERT INTO change_requests"
        "(requirement_id,requested_by,assigned_to,"
        "justification,changes_description,created_at,status) "
        "VALUES($1,$2,$3,$4,$5,NOW(),'pending') RETURNING id,created_at::text",
        req_id, user_id, nullable_int(assigned_to),
        justification, changes_desc);
    int cr_id = r[0][0].as<int>();
    txn.commit();

    auto opt = find_change_request_by_id(cr_id);
    return opt.value();
}

Paginated<ChangeRequest> Database::get_change_requests(
    int project_id, const std::string& status, int page, int per_page)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE req.project_id=" +
        std::to_string(project_id);
    if (!status.empty()) {
        where += " AND cr.status=" + txn.quote(status);
    }

    auto cnt = txn.exec(
        "SELECT COUNT(*) FROM change_requests cr "
        "JOIN requirements req ON req.id=cr.requirement_id "
        + where);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        "SELECT cr.id,cr.requirement_id,cr.status,"
        "cr.justification,cr.changes_description,"
        "cr.created_at::text,cr.resolved_at::text,"
        "cr.resolution_comment,"
        "COALESCE(req.system_id,'') AS req_sid,"
        "COALESCE(u1.username,'') AS req_by,"
        "COALESCE(u2.username,'') AS asgn_to,"
        "cr.requested_by,cr.assigned_to "
        "FROM change_requests cr "
        "JOIN requirements req ON req.id=cr.requirement_id "
        "LEFT JOIN users u1 ON u1.id=cr.requested_by "
        "LEFT JOIN users u2 ON u2.id=cr.assigned_to "
        + where +
        " ORDER BY cr.created_at DESC"
        " LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<ChangeRequest> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total > 0)
        ? static_cast<int>(std::ceil(
            static_cast<double>(total) / per_page))
        : 1;
    for (const auto& r : rows) {
        ChangeRequest cr;
        cr.id = r[0].as<int>();
        cr.requirement_id = r[1].as<int>();
        cr.status = safe_str(r, 2);
        cr.justification = safe_str(r, 3);
        cr.changes_description = safe_str(r, 4);
        cr.created_at = safe_str(r, 5);
        cr.resolved_at = safe_str(r, 6);
        cr.resolution_comment = safe_str(r, 7);
        cr.requirement_system_id = safe_str(r, 8);
        cr.requester_username = safe_str(r, 9);
        cr.assignee_username = safe_str(r, 10);
        cr.requested_by = safe_int(r, 11);
        cr.assigned_to = safe_int(r, 12);
        result.items.push_back(cr);
    }
    return result;
}

std::optional<ChangeRequest> Database::find_change_request_by_id(
    int id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto rows = txn.exec_params(
        "SELECT cr.id,cr.requirement_id,cr.status,"
        "cr.justification,cr.changes_description,"
        "cr.created_at::text,cr.resolved_at::text,"
        "cr.resolution_comment,"
        "COALESCE(req.system_id,'') AS req_sid,"
        "COALESCE(u1.username,'') AS req_by,"
        "COALESCE(u2.username,'') AS asgn_to,"
        "cr.requested_by,cr.assigned_to "
        "FROM change_requests cr "
        "JOIN requirements req ON req.id=cr.requirement_id "
        "LEFT JOIN users u1 ON u1.id=cr.requested_by "
        "LEFT JOIN users u2 ON u2.id=cr.assigned_to "
        "WHERE cr.id=$1", id);
    txn.commit();
    if (rows.empty()) return std::nullopt;
    const auto& r = rows[0];
    ChangeRequest cr;
    cr.id = r[0].as<int>();
    cr.requirement_id = r[1].as<int>();
    cr.status = safe_str(r, 2);
    cr.justification = safe_str(r, 3);
    cr.changes_description = safe_str(r, 4);
    cr.created_at = safe_str(r, 5);
    cr.resolved_at = safe_str(r, 6);
    cr.resolution_comment = safe_str(r, 7);
    cr.requirement_system_id = safe_str(r, 8);
    cr.requester_username = safe_str(r, 9);
    cr.assignee_username = safe_str(r, 10);
    cr.requested_by = safe_int(r, 11);
    cr.assigned_to = safe_int(r, 12);
    return cr;
}

void Database::update_change_request(int id,
    const std::string& status, const std::string& comment,
    int user_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE change_requests SET status=$1,"
        "resolution_comment=$2,resolved_at=NOW() WHERE id=$3",
        status, comment, id);
    txn.commit();
}

/* ===== Уведомления ===== */

void Database::create_notification(int user_id,
    const std::string& event_type, const std::string& message,
    const std::string& obj_type, int obj_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "INSERT INTO notifications"
        "(user_id,event_type,message,related_object_type,"
        "related_object_id,created_at) VALUES($1,$2,$3,$4,$5,NOW())",
        user_id, event_type, message, obj_type, obj_id);
    txn.commit();
}

Paginated<Notification> Database::get_notifications(int user_id,
    bool unread_only, int page, int per_page)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE user_id=" + std::to_string(user_id);
    if (unread_only) where += " AND is_read=false";

    auto cnt = txn.exec(
        "SELECT COUNT(*) FROM notifications " + where);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        "SELECT id,user_id,event_type,message,is_read,"
        "related_object_type,related_object_id,created_at::text "
        "FROM notifications " + where +
        " ORDER BY created_at DESC"
        " LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<Notification> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total > 0)
        ? static_cast<int>(std::ceil(
            static_cast<double>(total) / per_page))
        : 1;
    for (const auto& r : rows) {
        Notification n;
        n.id = r[0].as<int>();
        n.user_id = r[1].as<int>();
        n.event_type = safe_str(r, 2);
        n.message = safe_str(r, 3);
        n.is_read = r[4].is_null() ? false : r[4].as<bool>();
        n.related_object_type = safe_str(r, 5);
        n.related_object_id = safe_int(r, 6);
        n.created_at = safe_str(r, 7);
        result.items.push_back(n);
    }
    return result;
}

int Database::get_unread_count(int user_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    auto r = txn.exec_params(
        "SELECT COUNT(*) FROM notifications "
        "WHERE user_id=$1 AND is_read=false", user_id);
    txn.commit();
    return r[0][0].as<int>();
}

void Database::mark_notification_read(int id, int user_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "UPDATE notifications SET is_read=true "
        "WHERE id=$1 AND user_id=$2", id, user_id);
    txn.commit();
}

/* ===== Аудит ===== */

void Database::create_audit_log(int user_id,
    const std::string& action, const std::string& obj_type,
    const std::string& obj_id, const std::string& old_val,
    const std::string& new_val, const std::string& context,
    const std::string& ip)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);
    txn.exec_params(
        "INSERT INTO audit_log"
        "(user_id,action,object_type,object_id,old_value,"
        "new_value,context,ip_address,created_at) "
        "VALUES($1,$2,$3,$4,$5,$6,$7,$8,NOW())",
        user_id, action, obj_type, obj_id,
        old_val.empty() ? std::optional<std::string>{} : old_val,
        new_val.empty() ? std::optional<std::string>{} : new_val,
        context.empty() ? std::optional<std::string>{} : context,
        ip.empty() ? std::optional<std::string>{} : ip);
    txn.commit();
}

Paginated<AuditLog> Database::get_audit_logs(int page, int per_page,
    const std::string& obj_type, int user_id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    pqxx::work txn(*conn_);

    std::string where = "WHERE 1=1";
    if (!obj_type.empty()) {
        where += " AND al.object_type=" + txn.quote(obj_type);
    }
    if (user_id > 0) {
        where += " AND al.user_id=" + std::to_string(user_id);
    }

    auto cnt = txn.exec(
        "SELECT COUNT(*) FROM audit_log al " + where);
    int total = cnt[0][0].as<int>();
    int offset = (page - 1) * per_page;

    auto rows = txn.exec(
        "SELECT al.id,al.user_id,"
        "COALESCE(u.username,'') AS uname,"
        "al.action,al.object_type,al.object_id,"
        "al.old_value,al.new_value,al.context,"
        "al.ip_address,al.created_at::text "
        "FROM audit_log al "
        "LEFT JOIN users u ON u.id=al.user_id "
        + where +
        " ORDER BY al.created_at DESC"
        " LIMIT " + std::to_string(per_page) +
        " OFFSET " + std::to_string(offset));
    txn.commit();

    Paginated<AuditLog> result;
    result.total = total;
    result.page = page;
    result.per_page = per_page;
    result.pages = (total > 0)
        ? static_cast<int>(std::ceil(
            static_cast<double>(total) / per_page))
        : 1;
    for (const auto& r : rows) {
        AuditLog log;
        log.id = r[0].as<int>();
        log.user_id = safe_int(r, 1);
        log.username = safe_str(r, 2);
        log.action = safe_str(r, 3);
        log.object_type = safe_str(r, 4);
        log.object_id = safe_str(r, 5);
        log.old_value = safe_str(r, 6);
        log.new_value = safe_str(r, 7);
        log.context = safe_str(r, 8);
        log.ip_address = safe_str(r, 9);
        log.created_at = safe_str(r, 10);
        result.items.push_back(log);
    }
    return result;
}
