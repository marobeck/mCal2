/** habit.h
 * This library manages the relational database responsible for habit and habit management
 */
#pragma once

#include <sqlite3.h>
#include <time.h>

#include "defs.h"

typedef struct
{
    char task_uuid[UUID_LEN];
    char date[DATE_LEN]; // YYYY-MM-DD
} habit_t;

// ------------------------------------------ DATABASE --------------------------------------------

/// @brief Create new Habit Table (type)
/// @param uuid Unique idea for table
/// @param name Name of table
/// @param goal_flag Flags for which day of the week the task should be compelted
int habit_add_db(sqlite3 *db, const char *uuid, const char *name, int goal_flags);

/// @brief Appends a new entry to the database, with the type defined by the uuid of the habit table
/// @param habid_id The forign key of the entry defining what list the task is appended to
/// @param date Date to be added, formatted to ISO 8601 (Ex: 2025-05-05)
int habitentry_add_db(sqlite3 *db, const char *habit_id, time_t datetime);

/// @brief Removes a new entry if valid
/// @returns Error code. ESP_OK dictates that the entry doesn't exist, not whether an entry was
/// removed
int habitentry_remove_db(sqlite3 *db, const char *habit_id, time_t datetime);

// --------------------------------- Check if Habit is Relavent -----------------------------------

/// @brief Determines if an entry exists on the inculded date
/// @returns 1 on found, 0 on not found, -1 on error
int habitentry_is_completed_db(sqlite3 *db, const char *habit_id, time_t datetime);

/// @brief Checks if the user needs to complete a task on a weekday
/// @param wday Day of the week starting on Sunday (0-6)
///             Defined in tm struct
int habitentry_is_due_db(sqlite3 *db, const char *habit_id, time_t datetime);