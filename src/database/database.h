#pragma once

#include <sqlite3.h>
#include <vector>

#include "defs.h"

#include "timeblock.h"
#include "task.h"

// Utility: Generate UUID string (defined in database.cpp)
void generate_uuid(char *uuid_buf);

class Database
{
private:
    sqlite3 *db = nullptr;

public:
    // -------------------------------------- Initialization ----------------------------------------
    Database();
    ~Database();

    // --------------------------------------- Timeblock Data -----------------------------------------
    void insert_timeblock(const Timeblock &tb);
    void load_timeblocks(std::vector<Timeblock> &timeblocks);
    void update_timeblock(const Timeblock &tb);
    void delete_timeblock(const char *uuid);

    // ----------------------------------------- Task Data --------------------------------------------
    void insert_task(const Task &task);
    void load_tasks(Timeblock *timeblock);
    void update_task(const Task &task);
    void delete_task(const char *uuid);

    // -------------------------------------- Habit Entry Data ----------------------------------------
    void add_habit_entry(const char *task_uuid, const char *date_iso8601);
    void remove_habit_entry(const char *task_uuid, const char *date_iso8601);
    bool habit_entry_exists(const char *task_uuid, const char *date_iso8601);

    // Preview last N habit completions for a task and fill task.completed_days
    void load_habit_completion_preview(Task &task, const char *current_date_iso8601);
};