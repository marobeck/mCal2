#include <string.h>
#include <stdlib.h>

#include "database.h"
#include "log.h"

#include <uuid/uuid.h>

// Utility: Generate UUID string
void generate_uuid(char *uuid_buf)
{
    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, uuid_buf);
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

    const char *sql[] = {
        "PRAGMA foreign_keys = ON;",
        // Tables
        "CREATE TABLE IF NOT EXISTS timeblocks ( \
            uuid TEXT PRIMARY KEY, \
            status INTEGER NOT NULL, \
            name TEXT NOT NULL, \
            description TEXT, \
            day_frequency INTEGER NOT NULL, \
            duration INTEGER NOT NULL, \
            start INTEGER, \
            day_start INTEGER \
        ); \
        CREATE TABLE IF NOT EXISTS tasks( \
            uuid TEXT PRIMARY KEY, \
            timeblock_uuid TEXT,  \
            name TEXT NOT NULL, \
            description TEXT, \
            due_date INTEGER, \
            priority INTEGER NOT NULL, \
            status INTEGER NOT NULL, \
            goal_spec INTEGER NOT NULL, \
            FOREIGN KEY(timeblock_uuid) REFERENCES timeblocks(uuid) ON DELETE CASCADE); \
        CREATE TABLE IF NOT EXISTS habit_entries( \
            task_uuid TEXT, \
            date TEXT, \
            PRIMARY KEY(task_uuid, date), \
            FOREIGN KEY(task_uuid) REFERENCES tasks(uuid) ON DELETE CASCADE \
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
     */
    const char *sql = "INSERT INTO timeblocks VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
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

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Saved timeblock <%s> to database", tb.name);
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
        Timeblock tb;
        strncpy(tb.uuid, (const char *)sqlite3_column_text(stmt, 0), UUID_LEN);
        tb.status = static_cast<TimeblockStatus>(sqlite3_column_int(stmt, 1));
        tb.name = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        tb.desc = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        tb.day_frequency = GoalSpec::from_sql(sqlite3_column_int(stmt, 4));
        tb.duration = sqlite3_column_int64(stmt, 5);
        tb.start = sqlite3_column_int64(stmt, 6);
        tb.day_start = sqlite3_column_int64(stmt, 7);

        timeblocks.push_back(tb);
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
     */
    const char *sql = "UPDATE timeblocks SET status = ?, name = ?, description = ?, day_frequency = ?, duration = ?, start = ?, day_start = ? WHERE uuid = ?;";
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
    sqlite3_bind_text(stmt, 8, tb.uuid, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Updated timeblock <%s> in database", tb.name);
        return;
    }

    LOGE(TAG, "Failed to update timeblock <%s>: %s", tb.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::delete_timeblock(const char *uuid)
{
    const char *TAG = "DB::delete_timeblock";

    const char *sql = "DELETE FROM timeblocks WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Deleted timeblock with UUID: %s", uuid);
        return;
    }
    LOGE(TAG, "Failed to delete timeblock with UUID: %s", uuid);
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
     * status
     * goal_spec
     */
    const char *sql = "INSERT INTO tasks VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
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
    sqlite3_bind_int(stmt, 7, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 8, task.goal_spec.to_sql());

    // Execute
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Saved task <%s> to database", task.name);
        return;
    }

    LOGE(TAG, "Failed to insert task <%s>: %s", task.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::load_tasks(Timeblock *timeblock)
{
    const char *TAG = "DB::load_tasks";

    if (!timeblock)
    {
        LOGE(TAG, "Null timeblock provided to load_tasks");
        throw -1;
    }

    const char *sql = "SELECT * FROM tasks WHERE timeblock_uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGI(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, timeblock->uuid, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Task t;
        strncpy(t.uuid, (const char *)sqlite3_column_text(stmt, 0), UUID_LEN);
        strncpy(t.timeblock_uuid, (const char *)sqlite3_column_text(stmt, 1), UUID_LEN);
        t.name = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)));
        t.desc = strdup(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)));
        t.due_date = sqlite3_column_int64(stmt, 4);
        t.priority = static_cast<Priority>(sqlite3_column_int(stmt, 5));
        t.status = static_cast<TaskStatus>(sqlite3_column_int(stmt, 6));
        t.goal_spec = GoalSpec::from_sql(sqlite3_column_int(stmt, 7));

        // Sort between archive and active tasks
        if (t.status == TaskStatus::COMPLETE)
        {
            timeblock->append_archived(t);
        }
        else
        {
            timeblock->append(t);
        }
    }

    sqlite3_finalize(stmt);

    LOGI(TAG, "Loaded %zu tasks for timeblock <%s>", timeblock->tasks.size(), timeblock->name);
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
     */
    const char *sql = "UPDATE tasks SET name = ?, description = ?, due_date = ?, priority = ?, status = ?, goal_spec = ? WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, task.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, task.due_date);
    sqlite3_bind_int(stmt, 4, static_cast<int>(task.priority));
    sqlite3_bind_int(stmt, 5, static_cast<int>(task.status));
    sqlite3_bind_int(stmt, 6, task.goal_spec.to_sql());
    sqlite3_bind_text(stmt, 7, task.uuid, -1, SQLITE_STATIC);

    // Execute
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Updated task <%s> in database", task.name);
        return;
    }

    LOGE(TAG, "Failed to update task <%s>: %s", task.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::delete_task(const char *uuid)
{
    const char *TAG = "DB::delete_task";

    const char *sql = "DELETE FROM tasks WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Deleted task with UUID: %s", uuid);
        return;
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
        return;
    }
    LOGE(TAG, "Failed to add habit entry for task <%s> on date %s: %s", task_uuid, date_iso8601, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::remove_habit_entry(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::remove_habit_entry";

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