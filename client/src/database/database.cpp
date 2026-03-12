#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unordered_map>

#include "database.h"
#include "uuid.h"
#include "log.h"

#include <uuid/uuid.h>

// Utility: Generate UUID string
void generate_uuid(char *uuid_buf)
{
    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, uuid_buf);
}

// Utility: Get current epoch time
time_t get_current_epoch()
{
    return time(nullptr);
}

/** Database schema
 * Tables:
 * timeblocks:
 *  uuid (PK)           - unique identifier for timeblock
 *  status              - ONGOING, STOPPED, DONE, PINNED
 *  name                - user-defined name for timeblock
 *  description         - user-defined description for timeblock
 *  day_frequency       - encoded GoalSpec for which days this timeblock occurs on (0 = single event)
 *  duration            - length of timeblock in seconds
 *! Review: We could simplify the timeblock schema by just having a single 'start' field that represents either the start
 *!  time for single events or the epoch time of when a recurring event first started. The GoalSpec day_frequency can then
 *!  be used to determine which days the event occurs on, and we can calculate the next occurrence based on the current time
 *!  and the start time. This would eliminate the need for a separate 'day_start' field and reduce redundancy in the schema.
 *  start               - for single events, time since epoch of when event starts
 *  day_start           - For weekly events; Time since start of day (in seconds)
 *  completed_datetime  - time since epoch when timeblock was completed; 0 if not completed
 * tasks:
 *  uuid (PK)           - unique identifier for task
 *  timeblock_uuid (FK) - foreign key referencing parent timeblock
 *  name                - user-defined name for task
 *  description         - user-defined description for task
 *  due_date            - for non-habits: time since epoch of when task is due.
 *  status              - INCOMPLETE, IN_PROGRESS, HABIT, COMPLETE
 *  priority            - NONE, LOW, MEDIUM, HIGH, VERY_HIGH
 *  scope               - User-defined estimate of how much effort/time would be required to get this task done (XS -> XL)
 *  goal_spec           - encoded GoalSpec for habit tasks; 0 for non-habit tasks
 *  completed_datetime  - time since epoch when task was completed; 0 if not completed
 * habit_entries:
 *  task_uuid (PK, FK)  - foreign key referencing parent habit
 *  date (PK)           - ISO 8601 date string (YYYY-MM-DD) representing a day the habit was completed
 * entry_links (junction-table):
 *  parent_uuid (PK)    - UUID of parent entry
 *  child_uuid (PK)     - UUID of child entry
 *  link_type           - type of link (dependency, habit triggers)
 * change_receipts:
 *  uuid (PK)           - UUID of entry that was changed
 *  table_name          - name of table that was changed (timeblocks, tasks, habit_entries, entry_links)
 *  last_modified       - time since epoch of when the change was made
 */

/* -------------------------------------------------------------------------- */
/*                          Receipt Tracking Helpers                          */
/* -------------------------------------------------------------------------- */

void Database::clear_receipts()
{
    const char *TAG = "DB::clear_receipts";
    const char *sql[] = {
        "DELETE FROM timeblock_change_receipts;",
        "DELETE FROM task_change_receipts;",
        "DELETE FROM habit_entry_change_receipts;",
        "DELETE FROM entry_link_change_receipts;"};

    for (int i = 0; i < sizeof(sql) / sizeof(sql[0]); i++)
    {
        char *errmsg = NULL;
        int rc = sqlite3_exec(db, sql[i], 0, 0, &errmsg);
        if (rc != SQLITE_OK)
        {
            LOGE(TAG, "SQL error while clearing receipts: %s", errmsg);
            sqlite3_free(errmsg);
            throw rc;
        }
    }

    LOGI(TAG, "Cleared all change receipts");
}

// Store a snapshot of the given timeblock in the receipt table. If `deleted` is true
// the deleted_at column is set so the sync logic will know this row has been removed.
void Database::record_timeblock_receipt(const Timeblock &tb, bool deleted)
{
    const char *TAG = "DB::record_timeblock_receipt";
    const char *sql =
        "INSERT OR REPLACE INTO timeblock_change_receipts "
        "(uuid, status, name, description, day_frequency, duration, start, day_start, completed_datetime, modified_at, deleted_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, tb.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(tb.status));
    sqlite3_bind_text(stmt, 3, tb.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, tb.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, tb.day_frequency.to_sql());
    sqlite3_bind_int64(stmt, 6, tb.duration);
    sqlite3_bind_int64(stmt, 7, tb.start);
    sqlite3_bind_int64(stmt, 8, tb.day_start);
    sqlite3_bind_int64(stmt, 9, tb.completed_datetime);
    sqlite3_bind_int64(stmt, 10, get_current_epoch());
    if (deleted)
        sqlite3_bind_int64(stmt, 11, get_current_epoch());
    else
        sqlite3_bind_null(stmt, 11);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to record timeblock receipt for UUID <%s>: %s", tb.uuid, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

void Database::delete_timeblock_receipt(const Timeblock &tb)
{
    record_timeblock_receipt(tb, true);
}

void Database::record_task_receipt(const Task &task, bool deleted)
{
    const char *TAG = "DB::record_task_receipt";
    const char *sql =
        "INSERT OR REPLACE INTO task_change_receipts "
        "(uuid, timeblock_uuid, name, description, due_date, priority, scope, status, goal_spec, completed_datetime, modified_at, deleted_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task.timeblock_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, task.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, task.due_date);
    sqlite3_bind_int(stmt, 6, static_cast<int>(task.priority));
    sqlite3_bind_int(stmt, 7, static_cast<int>(task.scope));
    sqlite3_bind_int(stmt, 8, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 9, task.goal_spec.to_sql());
    sqlite3_bind_int64(stmt, 10, task.completed_datetime);
    sqlite3_bind_int64(stmt, 11, get_current_epoch());
    if (deleted)
        sqlite3_bind_int64(stmt, 12, get_current_epoch());
    else
        sqlite3_bind_null(stmt, 12);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to record task receipt for UUID <%s>: %s", task.uuid, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

void Database::delete_task_receipt(const Task &task)
{
    const char *TAG = "DB::record_task_receipt";
    const char *sql =
        "INSERT OR REPLACE INTO task_change_receipts "
        "(uuid, timeblock_uuid, name, description, due_date, priority, scope, status, goal_spec, completed_datetime, modified_at, deleted_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task.timeblock_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, task.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, task.due_date);
    sqlite3_bind_int(stmt, 6, static_cast<int>(task.priority));
    sqlite3_bind_int(stmt, 7, static_cast<int>(task.scope));
    sqlite3_bind_int(stmt, 8, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 9, task.goal_spec.to_sql());
    sqlite3_bind_int64(stmt, 10, task.completed_datetime);
    sqlite3_bind_int64(stmt, 11, get_current_epoch());
    sqlite3_bind_int64(stmt, 12, get_current_epoch()); // Set deleted_at to current time to indicate deletion

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to record task receipt for UUID <%s>: %s", task.uuid, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

void Database::record_habit_entry_receipt(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::record_habit_entry_receipt";
    const char *sql = "INSERT OR REPLACE INTO habit_entry_change_receipts (task_uuid, date, modified_at, deleted_at) VALUES (?, ?, ?, NULL);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, get_current_epoch());

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to record habit entry receipt for task UUID <%s> on date %s: %s", task_uuid, date_iso8601, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

void Database::delete_habit_entry_receipt(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::delete_habit_entry_receipt";
    const char *sql = "INSERT OR REPLACE INTO habit_entry_change_receipts (task_uuid, date, modified_at, deleted_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, get_current_epoch());
    sqlite3_bind_int64(stmt, 4, get_current_epoch());

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to delete habit entry receipt for task UUID <%s> on date %s: %s", task_uuid, date_iso8601, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

void Database::record_entry_link_receipt(const char *parent_uuid, const char *child_uuid, LinkType link_type)
{
    const char *TAG = "DB::record_entry_link_receipt";
    const char *sql =
        "INSERT OR REPLACE INTO entry_link_change_receipts "
        "(parent_uuid, child_uuid, link_type, modified_at, deleted_at) VALUES (?, ?, ?, ?, NULL);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, parent_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, child_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(link_type));
    sqlite3_bind_int64(stmt, 4, get_current_epoch());

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to record entry link receipt from <%s> to <%s>: %s", parent_uuid, child_uuid, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

void Database::delete_entry_link_receipt(const char *parent_uuid, const char *child_uuid, LinkType link_type)
{
    // simply reuse record routine with deleted flag
    const char *TAG = "DB::delete_entry_link_receipt";
    const char *sql =
        "INSERT OR REPLACE INTO entry_link_change_receipts "
        "(parent_uuid, child_uuid, link_type, modified_at, deleted_at) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, parent_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, child_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(link_type));
    sqlite3_bind_int64(stmt, 4, get_current_epoch());
    sqlite3_bind_int64(stmt, 5, get_current_epoch());

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to delete entry link receipt from <%s> to <%s>: %s", parent_uuid, child_uuid, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }
}

Database::Database()
{
    const char *TAG = "DB::init_db";

    int rc = sqlite3_open(DATABASE_PATH, &db);
    if (rc != SQLITE_OK)
    {
        LOGE(TAG, "Failed to open database: %s", sqlite3_errmsg(db));
        throw rc;
    }

    // migration: drop old receipt tables when upgrading from version <3
    int old_version = 0;
    {
        sqlite3_stmt *verStmt = nullptr;
        if (sqlite3_prepare_v2(db, "PRAGMA schema_version;", -1, &verStmt, nullptr) == SQLITE_OK)
        {
            if (sqlite3_step(verStmt) == SQLITE_ROW)
                old_version = sqlite3_column_int(verStmt, 0);
            sqlite3_finalize(verStmt);
        }
    }
    if (old_version < 3)
    {
        const char *dropSql[] = {
            "DROP TABLE IF EXISTS timeblock_change_receipts;",
            "DROP TABLE IF EXISTS task_change_receipts;",
            "DROP TABLE IF EXISTS habit_entry_change_receipts;",
            "DROP TABLE IF EXISTS entry_link_change_receipts;"};
        for (int i = 0; i < sizeof(dropSql) / sizeof(dropSql[0]); i++)
        {
            char *errmsg = nullptr;
            rc = sqlite3_exec(db, dropSql[i], 0, 0, &errmsg);
            if (rc != SQLITE_OK)
            {
                LOGW(TAG, "Failed to drop old receipt table: %s", errmsg);
                sqlite3_free(errmsg);
            }
        }
    }

    const char *sql[] = {
        "PRAGMA foreign_keys = ON;",
        // Increment this if we make breaking changes to the schema
        "PRAGMA schema_version = 3;", // Version 3: Denormalized sync receipts
        // Tables
        "CREATE TABLE IF NOT EXISTS timeblocks ( \
            uuid TEXT PRIMARY KEY, \
            status INTEGER NOT NULL, \
            name TEXT NOT NULL, \
            description TEXT, \
            day_frequency INTEGER NOT NULL, \
            duration INTEGER NOT NULL, \
            start INTEGER, \
            day_start INTEGER, \
            completed_datetime INTEGER \
        ); \
        CREATE TABLE IF NOT EXISTS tasks( \
            uuid TEXT PRIMARY KEY, \
            timeblock_uuid TEXT NOT NULL,  \
            name TEXT NOT NULL, \
            description TEXT, \
            due_date INTEGER, \
            priority INTEGER NOT NULL, \
            scope INTEGER NOT NULL, \
            status INTEGER NOT NULL, \
            goal_spec INTEGER NOT NULL, \
            completed_datetime INTEGER, \
            FOREIGN KEY(timeblock_uuid) REFERENCES timeblocks(uuid) ON DELETE CASCADE); \
        CREATE TABLE IF NOT EXISTS habit_entries( \
            task_uuid TEXT NOT NULL, \
            date TEXT NOT NULL, \
            PRIMARY KEY(task_uuid, date), \
            FOREIGN KEY(task_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE); \
        CREATE TABLE IF NOT EXISTS entry_links( \
            parent_uuid TEXT NOT NULL, \
            child_uuid TEXT NOT NULL, \
            link_type INTEGER NOT NULL, \
            PRIMARY KEY(parent_uuid, child_uuid), \
            FOREIGN KEY(parent_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE, \
            FOREIGN KEY(child_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE);",
        // Receipts tables for syncing with external clients (denormalized snapshots)
        "CREATE TABLE IF NOT EXISTS timeblock_change_receipts ( \
            uuid TEXT PRIMARY KEY, \
            status INTEGER NOT NULL, \
            name TEXT NOT NULL, \
            description TEXT, \
            day_frequency INTEGER NOT NULL, \
            duration INTEGER NOT NULL, \
            start INTEGER, \
            day_start INTEGER, \
            completed_datetime INTEGER, \
            modified_at INTEGER NOT NULL, \
            deleted_at INTEGER \
        ); \
        CREATE TABLE IF NOT EXISTS task_change_receipts ( \
            uuid TEXT PRIMARY KEY, \
            timeblock_uuid TEXT NOT NULL,  \
            name TEXT NOT NULL, \
            description TEXT, \
            due_date INTEGER, \
            priority INTEGER NOT NULL, \
            scope INTEGER NOT NULL, \
            status INTEGER NOT NULL, \
            goal_spec INTEGER NOT NULL, \
            completed_datetime INTEGER, \
            modified_at INTEGER NOT NULL, \
            deleted_at INTEGER \
        ); \
        CREATE TABLE IF NOT EXISTS habit_entry_change_receipts ( \
            task_uuid TEXT NOT NULL, \
            date TEXT NOT NULL, \
            modified_at INTEGER NOT NULL, \
            deleted_at INTEGER, \
            PRIMARY KEY(task_uuid, date) \
        ); \
        CREATE TABLE IF NOT EXISTS entry_link_change_receipts ( \
            parent_uuid TEXT NOT NULL, \
            child_uuid TEXT NOT NULL, \
            link_type INTEGER NOT NULL, \
            modified_at INTEGER NOT NULL, \
            deleted_at INTEGER, \
            PRIMARY KEY(parent_uuid, child_uuid) \
        ); \
        CREATE TABLE IF NOT EXISTS client_sync_state ( \
            id INTEGER PRIMARY KEY CHECK (id = 1), \
            last_server_version INTEGER NOT NULL DEFAULT 0 \
        );"};

    for (int i = 0; i < sizeof(sql) / sizeof(sql[0]); i++)
    {
        char *errmsg = NULL;
        rc = sqlite3_exec(db, sql[i], 0, 0, &errmsg);
        if (rc != SQLITE_OK)
        {
            LOGE(TAG, "SQL error: %s", errmsg);
            sqlite3_free(errmsg);
            sqlite3_close(db);
            throw rc;
        }
    }

    // Insert default client sync state if not exists
    rc = sqlite3_exec(db, "INSERT OR IGNORE INTO client_sync_state (id, last_server_version) VALUES (1, 0);", 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        LOGE(TAG, "Failed to initialize client sync state: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        throw rc;
    }

    LOGI(TAG, "Database initialized successfully");
}

Database::~Database()
{
    if (db)
    {
        sqlite3_close(db);
        db = nullptr;
    }
}

/* -------------------------------------------------------------------------- */
/*                               Timeblock data                               */
/* -------------------------------------------------------------------------- */

void Database::insert_timeblock(const Timeblock &tb)
{
    const char *TAG = "DB::insert_timeblock";

    /**
     * Timeblock fields:
     * uuid
     * status
     * name
     * description
     * day_frequency
     * duration
     * start
     * day_start
     * completed_datetime
     */
    const char *sql = "INSERT INTO timeblocks VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, tb.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(tb.status));
    sqlite3_bind_text(stmt, 3, tb.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, tb.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, tb.day_frequency.to_sql());
    sqlite3_bind_int64(stmt, 6, tb.duration);
    sqlite3_bind_int64(stmt, 7, tb.start);
    sqlite3_bind_int64(stmt, 8, tb.day_start);
    sqlite3_bind_int64(stmt, 9, tb.completed_datetime);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Saved timeblock <%s> to database", tb.name);
        // Record receipt for this change
        record_timeblock_receipt(tb, false);
        return;
    }

    LOGE(TAG, "Failed to insert timeblock <%s>: %s", tb.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::load_timeblocks(std::vector<Timeblock> &timeblocks)
{
    const char *TAG = "DB::load_timeblocks";

    const char *sql = "SELECT * FROM timeblocks;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Emplace a new Timeblock into the vector and populate it in-place to avoid
        // making a copy of a stack-allocated object (which could lead to shallow-copy
        // problems if Timeblock manages dynamic memory).
        timeblocks.emplace_back();
        Timeblock &tb = timeblocks.back();
        strncpy(tb.uuid.value, (const char *)sqlite3_column_text(stmt, 0), UUID_LEN);
        tb.status = static_cast<TimeblockStatus>(sqlite3_column_int(stmt, 1));
        tb.name = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        tb.desc = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        tb.day_frequency = GoalSpec::from_sql(sqlite3_column_int(stmt, 4));
        tb.duration = sqlite3_column_int64(stmt, 5);
        tb.start = sqlite3_column_int64(stmt, 6);
        tb.day_start = sqlite3_column_int64(stmt, 7);
        tb.completed_datetime = sqlite3_column_int64(stmt, 8);
    }

    sqlite3_finalize(stmt);
    LOGI(TAG, "Loaded %zu timeblocks from database", timeblocks.size());
}

void Database::update_timeblock(const Timeblock &tb)
{
    const char *TAG = "DB::update_timeblock";

    /**
     * Timeblock fields:
     * uuid
     * status
     * name
     * description
     * day_frequency
     * duration
     * start
     * day_start
     * completed_datetime
     */
    const char *sql = "UPDATE timeblocks SET status = ?, name = ?, description = ?, day_frequency = ?, duration = ?, start = ?, day_start = ?, completed_datetime = ? WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(tb.status));
    sqlite3_bind_text(stmt, 2, tb.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, tb.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, tb.day_frequency.to_sql());
    sqlite3_bind_int64(stmt, 5, tb.duration);
    sqlite3_bind_int64(stmt, 6, tb.start);
    sqlite3_bind_int64(stmt, 7, tb.day_start);
    sqlite3_bind_int64(stmt, 8, tb.completed_datetime);
    sqlite3_bind_text(stmt, 9, tb.uuid, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Updated timeblock <%s> in database", tb.name);
        // Record receipt for this change
        record_timeblock_receipt(tb, false);
        return;
    }

    LOGE(TAG, "Failed to update timeblock <%s>: %s", tb.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::upsert_timeblock(const Timeblock &tb)
{
    const char *TAG = "DB::upsert_timeblock";

    // INSERT with ON CONFLICT clause to perform an upsert based on the UUID primary key.
    // Removes need for a separate existence check before deciding to insert or update.
    const char *sql =
        "INSERT INTO timeblocks (uuid, status, name, description, day_frequency, duration, start, day_start, completed_datetime) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(uuid) DO UPDATE SET "
        "status = excluded.status, "
        "name = excluded.name, "
        "description = excluded.description, "
        "day_frequency = excluded.day_frequency, "
        "duration = excluded.duration, "
        "start = excluded.start, "
        "day_start = excluded.day_start, "
        "completed_datetime = excluded.completed_datetime;";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, tb.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(tb.status));
    sqlite3_bind_text(stmt, 3, tb.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, tb.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, tb.day_frequency.to_sql());
    sqlite3_bind_int64(stmt, 6, tb.duration);
    sqlite3_bind_int64(stmt, 7, tb.start);
    sqlite3_bind_int64(stmt, 8, tb.day_start);
    sqlite3_bind_int64(stmt, 9, tb.completed_datetime);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Upserted timeblock <%s>", tb.name);
        record_timeblock_receipt(tb, false);
        return;
    }

    LOGE(TAG, "Failed to upsert timeblock <%s>: %s", tb.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::delete_timeblock(const char *uuid, bool ignore_failure)
{
    const char *TAG = "DB::delete_timeblock";

    const char *sql =
        "DELETE FROM timeblocks "
        "WHERE uuid = ? "
        "RETURNING uuid, status, name, description, day_frequency, "
        "duration, start, day_start, completed_datetime;";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        Timeblock tb;

        strncpy(tb.uuid.value,
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)),
                UUID_LEN);

        tb.status = static_cast<TimeblockStatus>(sqlite3_column_int(stmt, 1));
        tb.name = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        tb.desc = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        tb.day_frequency = GoalSpec::from_sql(sqlite3_column_int(stmt, 4));
        tb.duration = sqlite3_column_int64(stmt, 5);
        tb.start = sqlite3_column_int64(stmt, 6);
        tb.day_start = sqlite3_column_int64(stmt, 7);
        tb.completed_datetime = sqlite3_column_int64(stmt, 8);

        sqlite3_finalize(stmt);

        LOGI(TAG, "Deleted timeblock with UUID: %s", uuid);

        delete_timeblock_receipt(tb);

        free(tb.name);
        free(tb.desc);

        return;
    }

    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        if (ignore_failure)
        {
            LOGI(TAG, "Delete ignored (timeblock not found): %s", uuid);
            return;
        }

        LOGE(TAG, "No timeblock found with UUID: %s", uuid);
        throw SQLITE_NOTFOUND;
    }

    LOGE(TAG, "Failed to delete timeblock %s: %s", uuid, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

/* -------------------------------------------------------------------------- */
/*                                  Task data                                 */
/* -------------------------------------------------------------------------- */

void Database::insert_task(const Task &task)
{
    const char *TAG = "DB::insert_task";

    /**
     * Task fields:
     * uuid
     * timeblock_uuid
     * name
     * description
     * due_date
     * priority
     * scope
     * status
     * goal_spec
     * completed_datetime
     */
    const char *sql = "INSERT INTO tasks VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task.timeblock_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, task.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, task.due_date);
    sqlite3_bind_int(stmt, 6, static_cast<int>(task.priority));
    sqlite3_bind_int(stmt, 7, static_cast<int>(task.scope));
    sqlite3_bind_int(stmt, 8, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 9, task.goal_spec.to_sql());
    sqlite3_bind_int64(stmt, 10, task.completed_datetime);

    // Execute
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Saved task <%s> to database", task.name);
        // Record receipt for this change
        record_task_receipt(task, false);
        return;
    }

    LOGE(TAG, "Failed to insert task <%s>: %s", task.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

/// @brief Load all tasks in database into provided hash map, keyed by UUID for O(1) access. Timeblocks will store pointers to their tasks for organization.
/// @param tasks Hash map to populate with tasks from database; should be empty when passed in
void Database::load_tasks(TaskHash &tasks)
{
    const char *TAG = "DB::load_tasks";

    if (!tasks.empty())
    {
        LOGW(TAG, "Provided task hash map is not empty, clearing before loading tasks from database.");
        tasks.clear();
    }

    const char *sql = "SELECT * FROM tasks;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGI(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Allocate the Task on the heap and populate it directly so we do not copy
        // a stack-allocated Task into the map (which could cause shallow-copying of
        // dynamically allocated members and lead to dangling pointers / double frees).
        std::unique_ptr<Task> tptr = std::make_unique<Task>();
        strncpy(tptr->uuid.value, (const char *)sqlite3_column_text(stmt, 0), UUID_LEN);
        strncpy(tptr->timeblock_uuid.value, (const char *)sqlite3_column_text(stmt, 1), UUID_LEN);
        tptr->name = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        tptr->desc = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        tptr->due_date = sqlite3_column_int64(stmt, 4);
        tptr->priority = static_cast<Priority>(sqlite3_column_int(stmt, 5));
        tptr->scope = static_cast<Scope>(sqlite3_column_int(stmt, 6));
        tptr->status = static_cast<TaskStatus>(sqlite3_column_int(stmt, 7));
        tptr->goal_spec = GoalSpec::from_sql(sqlite3_column_int(stmt, 8));
        tptr->completed_datetime = sqlite3_column_int64(stmt, 9);

        tasks[tptr->uuid] = std::move(tptr);
    }

    sqlite3_finalize(stmt);

    LOGI(TAG, "Loaded %zu tasks", tasks.size());
    return;
}

void Database::update_task(const Task &task)
{
    const char *TAG = "DB::update_task";

    /**
     * Task fields:
     * uuid
     * timeblock_uuid
     * name
     * description
     * due_date
     * priority
     * status
     * goal_spec
     * completed_datetime
     */
    const char *sql = "UPDATE tasks SET timeblock_uuid = ?, name = ?, description = ?, due_date = ?, priority = ?, scope = ?, status = ?, goal_spec = ?, completed_datetime = ? WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task.timeblock_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, task.due_date);
    sqlite3_bind_int(stmt, 5, static_cast<int>(task.priority));
    sqlite3_bind_int(stmt, 6, static_cast<int>(task.scope));
    sqlite3_bind_int(stmt, 7, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 8, task.goal_spec.to_sql());
    sqlite3_bind_int64(stmt, 9, task.completed_datetime);
    sqlite3_bind_text(stmt, 10, task.uuid, -1, SQLITE_STATIC);

    // Execute
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Updated task <%s> in database", task.name);
        // Record receipt for this change
        record_task_receipt(task, false);
        return;
    }

    LOGE(TAG, "Failed to update task <%s>: %s", task.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::upsert_task(const Task &task)
{
    const char *TAG = "DB::upsert_task";

    // INSERT with ON CONFLICT clause to perform an upsert based on the UUID primary key.
    // Removes need for a separate existence check before deciding to insert or update.
    const char *sql =
        "INSERT INTO tasks (uuid, timeblock_uuid, name, description, due_date, priority, scope, status, goal_spec, completed_datetime) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(uuid) DO UPDATE SET "
        "timeblock_uuid = excluded.timeblock_uuid, "
        "name = excluded.name, "
        "description = excluded.description, "
        "due_date = excluded.due_date, "
        "priority = excluded.priority, "
        "scope = excluded.scope, "
        "status = excluded.status, "
        "goal_spec = excluded.goal_spec, "
        "completed_datetime = excluded.completed_datetime;";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task.uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task.timeblock_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, task.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, task.due_date);
    sqlite3_bind_int(stmt, 6, static_cast<int>(task.priority));
    sqlite3_bind_int(stmt, 7, static_cast<int>(task.scope));
    sqlite3_bind_int(stmt, 8, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 9, task.goal_spec.to_sql());
    sqlite3_bind_int64(stmt, 10, task.completed_datetime);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Upserted task <%s>", task.name);
        record_task_receipt(task, false);
        return;
    }

    LOGE(TAG, "Failed to upsert task <%s>: %s", task.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::delete_task(const char *uuid, bool ignore_failure)
{
    const char *TAG = "DB::delete_task";

    const char *sql = "DELETE FROM tasks "
                      "WHERE uuid = ?"
                      "RETURNING uuid, timeblock_uuid, name, description, "
                      "due_date, priority, scope, status, goal_spec, completed_datetime;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        Task task;

        strncpy(task.uuid.value,
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)),
                UUID_LEN);

        strncpy(task.timeblock_uuid.value,
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
                UUID_LEN);

        task.name = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        task.desc = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        task.due_date = sqlite3_column_int64(stmt, 4);
        task.priority = static_cast<Priority>(sqlite3_column_int(stmt, 5));
        task.scope = static_cast<Scope>(sqlite3_column_int(stmt, 6));
        task.status = static_cast<TaskStatus>(sqlite3_column_int(stmt, 7));
        task.goal_spec = GoalSpec::from_sql(sqlite3_column_int(stmt, 8));
        task.completed_datetime = sqlite3_column_int64(stmt, 9);

        sqlite3_finalize(stmt);

        LOGI(TAG, "Deleted task with UUID: %s", uuid);

        delete_task_receipt(task);

        return;
    }

    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        if (ignore_failure)
        {
            LOGI(TAG, "Delete ignored (task not found): %s", uuid);
            return;
        }

        LOGE(TAG, "No task found with UUID: %s", uuid);
        throw SQLITE_NOTFOUND;
    }

    LOGE(TAG, "Failed to delete task with UUID: %s", uuid);
    throw sqlite3_errcode(db);
}

/* -------------------------------------------------------------------------- */
/*                              Habit entry data                              */
/* -------------------------------------------------------------------------- */

void Database::add_habit_entry(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::add_habit_entry";

    const char *sql = "INSERT OR IGNORE INTO habit_entries (task_uuid, date) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Added habit entry for task <%s> on date %s", task_uuid, date_iso8601);
        // Record receipt for this change
        record_habit_entry_receipt(task_uuid, date_iso8601);
        return;
    }
    LOGE(TAG, "Failed to add habit entry for task <%s> on date %s: %s", task_uuid, date_iso8601, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::remove_habit_entry(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::remove_habit_entry";

    // Check if the habit entry exists before trying to delete it, so we can return early without error if it doesn't exist
    if (!habit_entry_exists(task_uuid, date_iso8601))
    {
        LOGW(TAG, "Habit entry for task <%s> on date %s does not exist, nothing to remove", task_uuid, date_iso8601);
        return;
    }

    // We don't need to check if the deletion actually removed a row since either way the end result is that the habit entry doesn't exist, which is what we want.
    // We just need to make sure to record a deletion receipt if it did exist so that external clients can sync this change.
    const char *sql = "DELETE FROM habit_entries WHERE task_uuid = ? AND date = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Removed habit entry for task <%s> on date %s", task_uuid, date_iso8601);
        // Record deletion receipt for this change
        delete_habit_entry_receipt(task_uuid, date_iso8601);
        return;
    }

    LOGE(TAG, "Failed to remove habit entry for task <%s> on date %s: %s", task_uuid, date_iso8601, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

bool Database::habit_entry_exists(const char *task_uuid, const char *date_iso8601)
{
    const char *sql = "SELECT 1 FROM habit_entries WHERE task_uuid = ? AND date = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return true;
    }

    sqlite3_finalize(stmt);
    return false;
}

// Preview habit completions over the last N days for a task and fill task.completed_days
void Database::load_habit_completion_preview(Task &task, const char *current_date_iso8601)
{
    const char *TAG = "DB::load_habit_completion_preview";

    size_t len = sizeof(task.completed_days) / sizeof(task.completed_days[0]);

    // Recursive CTE to get last N days and check for habit entries
    const char *sql =
        R"(WITH RECURSIVE days(n, d) AS (
      SELECT 0, date(?)
      UNION ALL
      SELECT n+1, date(d, '-1 day') FROM days WHERE n+1 < ?
    )
    SELECT n, d, CASE WHEN he.date IS NOT NULL THEN 1 ELSE 0 END AS done
    FROM days
    LEFT JOIN habit_entries he
      ON he.task_uuid = ? AND he.date = d
    ORDER BY n;)";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    // Bind params
    sqlite3_bind_text(stmt, 1, current_date_iso8601, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(len));
    sqlite3_bind_text(stmt, 3, task.uuid, -1, SQLITE_STATIC);

    size_t index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && index < len)
    {
        // n = sqlite3_column_int(stmt, 0)  (should equal index)
        const unsigned char *date_text = sqlite3_column_text(stmt, 1); // "YYYY-MM-DD"
        int done = sqlite3_column_int(stmt, 2);                        // 0 or 1

        /* Get time_t from date string
        if (date_text && done)
        {
            struct tm tm = {};
            if (strptime(reinterpret_cast<const char *>(date_text), "%Y-%m-%d", &tm))
            {
                time_t t = mktime(&tm);
                task.completed_days[index] = t; // index 0 => current date
            }
            else
            {
                LOGW(TAG, "Failed to parse date string %s", date_text);
                task.completed_days[index] = 0;
            }
        }
        else
        {
            task.completed_days[index] = 0;
        }
        */
        if (done)
            task.completed_days[index] = TaskStatus::COMPLETE;
        else
        {
            if (task.completed_days[index] != TaskStatus::IN_PROGRESS)
                task.completed_days[index] = TaskStatus::INCOMPLETE;
        }
        ++index;
    }

    sqlite3_finalize(stmt);

    // LOGI(TAG, "Loaded %zu habit completions for task <%s>", index, task.name);
}

void Database::get_habit_entries(const char *task_uuid, std::vector<time_t> &outDates)
{
    const char *TAG = "DB::get_habit_entries";

    const char *sql = "SELECT date FROM habit_entries WHERE task_uuid = ? ORDER BY date ASC;";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *date_text = sqlite3_column_text(stmt, 0); // "YYYY-MM-DD"
        if (!date_text)
            continue;

        struct tm tm = {};
        if (strptime(reinterpret_cast<const char *>(date_text), "%Y-%m-%d", &tm))
        {
            time_t t = mktime(&tm);
            outDates.push_back(t);
        }
        else
        {
            LOGW(TAG, "Failed to parse date string %s", date_text);
        }
    }

    sqlite3_finalize(stmt);
}

/* -------------------------------------------------------------------------- */
/*                               Entry link data                              */
/* -------------------------------------------------------------------------- */

void Database::add_entry_link(const char *parent_uuid, const char *child_uuid, LinkType link_type)
{
    const char *TAG = "DB::add_entry_link";

    const char *sql = "INSERT OR IGNORE INTO entry_links (parent_uuid, child_uuid, link_type) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, parent_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, child_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(link_type));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Added entry link from <%s> to <%s> with link type %d", parent_uuid, child_uuid, static_cast<int>(link_type));
        record_entry_link_receipt(parent_uuid, child_uuid, link_type);
        return;
    }
    LOGE(TAG, "Failed to add entry link from <%s> to <%s>: %s", parent_uuid, child_uuid, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::remove_entry_link(const char *parent_uuid, const char *child_uuid, LinkType link_type)
{
    const char *TAG = "DB::remove_entry_link";

    const char *sql = "DELETE FROM entry_links WHERE parent_uuid = ? AND child_uuid = ? AND link_type = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, parent_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, child_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, static_cast<int>(link_type));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Removed entry link from <%s> to <%s> with link type %d", parent_uuid, child_uuid, static_cast<int>(link_type));
        delete_entry_link_receipt(parent_uuid, child_uuid, link_type);
        return;
    }
    LOGE(TAG, "Failed to remove entry link from <%s> to <%s>: %s", parent_uuid, child_uuid, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::remove_all_links_for_task(const char *task_uuid)
{
    const char *TAG = "DB::remove_all_links_for_task";

    // Delete all links where the task is either the parent or child, and return the deleted links so we can record receipts for them
    const char *sql =
        "DELETE FROM entry_links "
        "WHERE parent_uuid = ? OR child_uuid = ? "
        "RETURNING parent_uuid, child_uuid, link_type;";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task_uuid, -1, SQLITE_STATIC);

    int rc;
    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const char *parent_uuid =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *child_uuid =
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        LinkType link_type = static_cast<LinkType>(sqlite3_column_int(stmt, 2));

        delete_entry_link_receipt(parent_uuid, child_uuid, link_type);

        count++;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        LOGE(TAG, "Failed to remove entry links for task <%s>: %s",
             task_uuid, sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    LOGI(TAG, "Removed %d entry links for task <%s>", count, task_uuid);
}

void Database::get_linked_entries(const char *uuid, LinkType link_type, std::vector<char *> &outLinkedUuids)
{
    const char *TAG = "DB::get_linked_entries";

    const char *sql = "SELECT child_uuid FROM entry_links WHERE parent_uuid = ? AND link_type = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(link_type));

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *linked_uuid = sqlite3_column_text(stmt, 0);
        if (linked_uuid)
        {
            outLinkedUuids.push_back(strdup(reinterpret_cast<const char *>(linked_uuid)));
            LOGI(TAG, "Found linked entry <%s> with link type %d", linked_uuid, static_cast<int>(link_type));
        }
    }

    sqlite3_finalize(stmt);
}