#include <string.h>
#include <stdlib.h>

#include "database.h"
#include "log.h"

void Database::init_db()
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

/* -------------------------------------------------------------------------- */
/*                               Timeblock data                               */
/* -------------------------------------------------------------------------- */

void Database::db_insert_timeblock(const Timeblock &tb)
{
    const char *TAG = "DB::db_insert_timeblock";

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
    sqlite3_bind_int(stmt, 2, tb.status);
    sqlite3_bind_text(stmt, 3, tb.name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, tb.desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, tb.day_frequency.to_sql());
    sqlite3_bind_int64(stmt, 6, tb.duration);
    sqlite3_bind_int64(stmt, 7, tb.start);
    sqlite3_bind_int64(stmt, 8, tb.day_start);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_OK)
    {
        LOGI(TAG, "Saved timeblock <%s> to database", tb.name);
        return;
    }

    LOGE(TAG, "Failed to insert timeblock <%s>: %s", tb.name, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::db_load_timeblocks(std::vector<Timeblock> &timeblocks)
{
    const char *TAG = "DB::db_load_timeblocks";

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
        tb.status = static_cast<TIMEBLOCK_STATUS>(sqlite3_column_int(stmt, 1));
        strncpy(tb.name, (const char *)sqlite3_column_text(stmt, 2), NAME_LEN);
        strncpy(tb.desc, (const char *)sqlite3_column_text(stmt, 3), DESC_LEN);
        tb.day_frequency = GoalSpec::from_sql(sqlite3_column_int(stmt, 4));
        tb.duration = sqlite3_column_int64(stmt, 5);
        tb.start = sqlite3_column_int64(stmt, 6);
        tb.day_start = sqlite3_column_int64(stmt, 7);

        timeblocks.push_back(tb);
    }

    sqlite3_finalize(stmt);
    LOGI(TAG, "Loaded %zu timeblocks from database", timeblocks.size());
}

void Database::db_delete_timeblock(const char *uuid)
{
    const char *TAG = "DB::db_delete_timeblock";

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

void Database::db_insert_task(const Task &task)
{
    const char *TAG = "DB::db_insert_task";

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

void Database::db_load_tasks(Timeblock *timeblock)
{
    const char *TAG = "DB::db_load_tasks";

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
        strncpy(t.name, (const char *)sqlite3_column_text(stmt, 2), NAME_LEN);
        strncpy(t.desc, (const char *)sqlite3_column_text(stmt, 3), DESC_LEN);
        t.due_date = sqlite3_column_int64(stmt, 4);
        t.priority = static_cast<Priority>(sqlite3_column_int(stmt, 5));
        t.status = static_cast<TaskStatus>(sqlite3_column_int(stmt, 6));
        t.goal_spec = GoalSpec::from_sql(sqlite3_column_int(stmt, 7));
    }

    sqlite3_finalize(stmt);
    return;
}

void Database::db_update_task_status(const char *uuid, TaskStatus new_status)
{
    const char *TAG = "DB::db_update_task_status";

    int status_int = static_cast<int>(new_status);

    const char *sql = "UPDATE tasks SET status = ? WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        LOGE(TAG, "Failed to prepare statement: %s", sqlite3_errmsg(db));
        throw sqlite3_errcode(db);
    }

    sqlite3_bind_int(stmt, 1, status_int);
    sqlite3_bind_text(stmt, 2, uuid, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
    {
        LOGI(TAG, "Updated task <%s> status to %d", uuid, status_int);
        return;
    }

    LOGE(TAG, "Failed to update task <%s> status: %s", uuid, sqlite3_errmsg(db));
    throw sqlite3_errcode(db);
}

void Database::db_delete_task(const char *uuid)
{
    const char *TAG = "DB::db_delete_task";

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

void Database::db_add_habit_entry(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::db_add_habit_entry";

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

void Database::db_remove_habit_entry(const char *task_uuid, const char *date_iso8601)
{
    const char *TAG = "DB::db_remove_habit_entry";

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

bool Database::db_habit_entry_exists(const char *task_uuid, const char *date_iso8601)
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
