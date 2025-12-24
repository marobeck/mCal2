#pragma once

#include <sqlite3.h>
#include <vector>
#include <uuid/uuid.h>

#include "defs.h"

#include "timeblock.h"
#include "task.h"

// Utility: Generate UUID string
void generate_uuid(char *uuid_buf)
{
    uuid_t binuuid;
    uuid_generate_random(binuuid);
    uuid_unparse_lower(binuuid, uuid_buf);
}

class Database
{
private:
    sqlite3 *db = nullptr;

public:
    // -------------------------------------- Initialization ----------------------------------------
    void init_db();

    // --------------------------------------- Timeblock Data -----------------------------------------
    void db_insert_timeblock(const Timeblock &tb);
    void db_load_timeblocks(std::vector<Timeblock> &timeblocks);
    void db_delete_timeblock(const char *uuid);

    // ----------------------------------------- Task Data --------------------------------------------
    void db_insert_task(const Task &task);
    void db_load_tasks(Timeblock *timeblock);
    void db_update_task_status(const char *uuid, TaskStatus new_status);
    void db_delete_task(const char *uuid);

    // -------------------------------------- Habit Entry Data ----------------------------------------
    void db_add_habit_entry(const char *task_uuid, const char *date_iso8601);
    void db_remove_habit_entry(const char *task_uuid, const char *date_iso8601);
    bool db_habit_entry_exists(const char *task_uuid, const char *date_iso8601);
};