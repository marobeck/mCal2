#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "../defs.h"

#include "timeblock.h"
#include "task.h"

int init_db(sqlite3 **db);

// --------------------------------------- Timeblock Data -----------------------------------------
int db_insert_timeblock(sqlite3 *db, const timeblock_t *tb);
int db_load_timeblocks(sqlite3 *db, timeblock_t **timeblocks, int *count);
int db_delete_timeblock(sqlite3 *db, const char *uuid);

// ----------------------------------------- Task Data --------------------------------------------
int db_insert_task(sqlite3 *db, const Task *task);
int db_load_tasks(sqlite3 *db, const char *timeblock_uuid, task_t **tasks, int *count);
int db_update_task_status(sqlite3 *db, const char *uuid, TASK_STATUS new_status);
int db_delete_task(sqlite3 *db, const char *uuid);

// -------------------------------------- Habit Entry Data ----------------------------------------
int db_add_habit_entry(sqlite3 *db, const char *task_uuid, const char *date_iso8601);
int db_habit_entry_exists(sqlite3 *db, const char *task_uuid, const char *date_iso8601, int *exists);
int db_remove_habit_entry(sqlite3 *db, const char *task_uuid, const char *date_iso8601);

#endif