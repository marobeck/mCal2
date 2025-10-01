#include <string.h>
#include <stdlib.h>

#include "database.h"
#include "../log.h"

int init_db(sqlite3 **db)
{
    const char *TAG = "DB::init_db";

    int rc = sqlite3_open(DATABASE_PATH, db);
    if (rc != SQLITE_OK)
    {
        LOGE(TAG, "Failed to open database: %s", sqlite3_errmsg(*db));
        return rc;
    }

    const char *sql[] = {
        "PRAGMA foreign_keys = ON;",
        // Tables
        "CREATE TABLE IF NOT EXISTS timeblocks ( \
            uuid TEXT PRIMARY KEY, \
            name TEXT NOT NULL, \
            description TEXT, \
            weekly INTEGER NOT NULL, \
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
            priority INTEGER, \
            completion INTEGER, \
            frequency INTEGER, \
            day_frequency INTEGER, \
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
        rc = sqlite3_exec(*db, sql[i], 0, 0, &errmsg);
        if (rc != SQLITE_OK)
        {
            LOGE(TAG, "SQL error: %s", errmsg);
            sqlite3_free(errmsg);
            sqlite3_close(*db);
            return rc;
        }
    }

    LOGI(TAG, "Database initialized successfully");
    return SQLITE_OK;
}

// --------------------------------------- Timeblock Data -----------------------------------------

int db_insert_timeblock(sqlite3 *db, const timeblock_t *tb)
{
    const char *sql = "INSERT INTO timeblocks VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, tb->uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, tb->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, tb->desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, tb->day_frequency);
    sqlite3_bind_int64(stmt, 5, tb->duration);
    sqlite3_bind_int64(stmt, 6, tb->start);
    sqlite3_bind_int64(stmt, 7, tb->day_start);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_OK)
    {
        LOGI("DB::db_insert_timeblock", "Saved timeblock <%s> to database", tb->name);
    }
    return rc;
}

int db_load_timeblocks(sqlite3 *db, timeblock_t **timeblocks, int *count)
{
    const char *sql = "SELECT * FROM timeblocks;";
    sqlite3_stmt *stmt;
    *count = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    timeblock_t *list = malloc(16 * sizeof(timeblock_t)); // Start with 16
    int cap = 16;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (*count >= cap)
        {
            cap *= 2;
            list = realloc(list, cap * sizeof(timeblock_t));
        }

        timeblock_t *tb = &list[*count];
        strncpy(tb->uuid, (const char *)sqlite3_column_text(stmt, 0), UUID_LEN);
        strncpy(tb->name, (const char *)sqlite3_column_text(stmt, 1), NAME_LEN);
        strncpy(tb->desc, (const char *)sqlite3_column_text(stmt, 2), DESC_LEN);
        tb->day_frequency = sqlite3_column_int(stmt, 3);
        tb->duration = sqlite3_column_int64(stmt, 4);
        tb->start = sqlite3_column_int64(stmt, 5);
        tb->day_start = sqlite3_column_int64(stmt, 6);

        (*count)++;
    }

    sqlite3_finalize(stmt);
    *timeblocks = list;
    return SQLITE_OK;
}

int db_delete_timeblock(sqlite3 *db, const char *uuid)
{
    const char *sql = "DELETE FROM timeblocks WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

// ----------------------------------------- Task Data --------------------------------------------

int db_insert_task(sqlite3 *db, const task_t *task)
{
    const char *sql = "INSERT INTO tasks VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, task->uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, task->timeblock_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, task->desc, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, task->due_date);
    sqlite3_bind_int(stmt, 6, task->priority);
    sqlite3_bind_int(stmt, 7, task->status);
    sqlite3_bind_int(stmt, 8, task->frequency);
    sqlite3_bind_int(stmt, 9, task->day_frequency);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

int db_load_tasks(sqlite3 *db, const char *timeblock_uuid, task_t **tasks, int *count)
{
    const char *sql = "SELECT * FROM tasks WHERE timeblock_uuid = ?;";
    sqlite3_stmt *stmt;
    *count = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, timeblock_uuid, -1, SQLITE_STATIC);

    task_t *list = malloc(16 * sizeof(task_t));
    int cap = 16;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (*count >= cap)
        {
            cap *= 2;
            list = realloc(list, cap * sizeof(task_t));
        }

        task_t *t = &list[*count];
        strncpy(t->uuid, (const char *)sqlite3_column_text(stmt, 0), UUID_LEN);
        strncpy(t->timeblock_uuid, (const char *)sqlite3_column_text(stmt, 1), UUID_LEN);
        strncpy(t->name, (const char *)sqlite3_column_text(stmt, 2), NAME_LEN);
        strncpy(t->desc, (const char *)sqlite3_column_text(stmt, 3), DESC_LEN);
        t->due_date = sqlite3_column_int64(stmt, 4);
        t->priority = sqlite3_column_int(stmt, 5);
        t->status = sqlite3_column_int(stmt, 6);
        t->frequency = sqlite3_column_int(stmt, 7);
        t->day_frequency = sqlite3_column_int(stmt, 8);

        (*count)++;
    }

    sqlite3_finalize(stmt);
    *tasks = list;
    return SQLITE_OK;
}

int db_update_task_status(sqlite3 *db, const char *uuid, TASK_STATUS new_status)
{
    // Handle deletion shortcut
    if (new_status == MFD)
    {
        return db_delete_task(db, uuid);
    }

    const char *sql = "UPDATE tasks SET completion = ? WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_int(stmt, 1, new_status);
    sqlite3_bind_text(stmt, 2, uuid, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

int db_delete_task(sqlite3 *db, const char *uuid)
{
    const char *sql = "DELETE FROM tasks WHERE uuid = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

// -------------------------------------- Habit Entry Data ----------------------------------------

int db_add_habit_entry(sqlite3 *db, const char *task_uuid, const char *date_iso8601)
{
    const char *sql = "INSERT OR IGNORE INTO habit_entries (task_uuid, date) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

int db_remove_habit_entry(sqlite3 *db, const char *task_uuid, const char *date_iso8601)
{
    const char *sql = "DELETE FROM habit_entries WHERE task_uuid = ? AND date = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

int db_habit_entry_exists(sqlite3 *db, const char *task_uuid, const char *date_iso8601, int *exists)
{
    const char *sql = "SELECT 1 FROM habit_entries WHERE task_uuid = ? AND date = ?;";
    sqlite3_stmt *stmt;
    *exists = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
        return sqlite3_errcode(db);

    sqlite3_bind_text(stmt, 1, task_uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, date_iso8601, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        *exists = 1;
    }

    sqlite3_finalize(stmt);
    return SQLITE_OK;
}
