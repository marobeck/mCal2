#pragma once

#include <sqlite3.h>
#include <vector>
#include <memory>

#include "uuid.h"
#include "timeblock.h"
#include "task.h"

#define DATABASE_PATH "database.db"

// Utility: Generate UUID string (defined in database.cpp)
void generate_uuid(char *uuid_buf);

// --- Hash function for UUID to allow use in unordered_map ---
namespace std
{
    template <>
    struct hash<UUID>
    {
        std::size_t operator()(const UUID &u) const noexcept
        {
            // FNV-1a hash (fast and good for fixed strings)
            std::size_t h = 1469598103934665603ull;
            for (int i = 0; i < UUID_LEN - 1 && u.value[i]; ++i)
            {
                h ^= static_cast<unsigned char>(u.value[i]);
                h *= 1099511628211ull;
            }
            return h;
        }
    };
}

class Database
{
    friend class Synchronizer; // Allow synchronizer to access private members for sync operations
private:
    sqlite3 *db = nullptr;

    // Helper functions for receipt tracking
    // For timeblocks and tasks we store a snapshot of the object so receipts are self-contained.
    void record_timeblock_receipt(const Timeblock &tb, bool deleted = false);
    void delete_timeblock_receipt(const Timeblock &tb); // convenience wrapper
    void record_task_receipt(const Task &task, bool deleted = false);
    void delete_task_receipt(const Task &task); // convenience wrapper
    void record_habit_entry_receipt(const char *task_uuid, const char *date_iso8601);
    void delete_habit_entry_receipt(const char *task_uuid, const char *date_iso8601);
    void record_entry_link_receipt(const char *parent_uuid, const char *child_uuid, LinkType link_type);
    void delete_entry_link_receipt(const char *parent_uuid, const char *child_uuid, LinkType link_type);

public:
    // -------------------------------------- Initialization ----------------------------------------
    Database();
    ~Database();

    // ---------------------------------------- Receipt data ------------------------------------------
    void clear_receipts(); // Clear all receipts (on completed sync)

    // --------------------------------------- Timeblock Data -----------------------------------------
    void insert_timeblock(const Timeblock &tb);
    void load_timeblocks(std::vector<Timeblock> &timeblocks);
    void update_timeblock(const Timeblock &tb);
    void upsert_timeblock(const Timeblock &tb);
    void delete_timeblock(const char *uuid, bool ignore_failure = false);

    // ----------------------------------------- Task Data --------------------------------------------
    void insert_task(const Task &task);
    void load_tasks(TaskHash &tasks);
    void update_task(const Task &task);
    void upsert_task(const Task &task);
    void delete_task(const char *uuid, bool ignore_failure = false);

    // -------------------------------------- Habit Entry Data ----------------------------------------
    void add_habit_entry(const char *task_uuid, const char *date_iso8601);
    void upsert_habit_entry(const char *task_uuid, const char *date_iso8601);
    void remove_habit_entry(const char *task_uuid, const char *date_iso8601);
    bool habit_entry_exists(const char *task_uuid, const char *date_iso8601);

    // Preview last N habit completions for a task and fill task.completed_days
    void load_habit_completion_preview(Task &task, const char *current_date_iso8601);
    // Load all habit entry dates for a task (ISO date strings -> time_t)
    void get_habit_entries(const char *task_uuid, std::vector<time_t> &outDates);

    // -------------------------------------- Entry Link Data ----------------------------------------
    void add_entry_link(const char *parent_uuid, const char *child_uuid, LinkType link_type);
    // void upsert_entry_link(const char *parent_uuid, const char *child_uuid, LinkType link_type);
    void remove_entry_link(const char *parent_uuid, const char *child_uuid, LinkType link_type);
    void remove_all_links_for_task(const char *task_uuid);
    void remove_all_child_links_for_task(const char *task_uuid);
    void get_linked_entries(const char *uuid, LinkType link_type, std::vector<char *> &outLinkedUuids);
};