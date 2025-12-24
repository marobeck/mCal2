#pragma once

#include <ctime>
#include <string>
#include <sqlite3.h>

#include "defs.h"
#include "goalspec.h"

enum class Priority
{
    NONE = -1,
    VERY_LOW = 0,
    LOW = 2,
    MEDIUM = 3,
    HIGH = 5,
    VERY_HIGH = 7
};

enum class TaskStatus
{
    INCOMPLETE = 0,
    IN_PROGRESS = 2,
    COMPLETE = 1,
    HABIT = 3
};

/** Task Struct
 * UUID string used for id to match with JSON format.
 */
class Task
{
public:
    // --- Location ---
    // Used for database fetching
    char uuid[UUID_LEN]; // UUID string of entry
    char timeblock_uuid[UUID_LEN];

    // --- Task Urgency ---

    time_t due_date = 0;                        // Due date, 0 = undated
    Priority priority = Priority::NONE;         // Priority
    TaskStatus status = TaskStatus::INCOMPLETE; // Completion status of the entry

    // --- Habit parameters ---

    GoalSpec goal_spec;        // Goal specification for habit tasks
    time_t completed_days[10]; // cache of the last 10 timestamps of days where task was complete

    // --- Descriptive fields ---
    char *name = NULL;   // Title of entry
    char *desc = NULL;   // Verbose description of entry
    Task *prereq = NULL; // Task that must be completed prior to completing this one.

    // --- Constructors ---

    Task() = default;
    Task(const char *name_, const char *desc_, Priority priority_ = Priority::NONE, time_t due_date_ = 0, char frequency_ = 0);

    // --- Get parameters ---
    /**
     * Get urgency of a task
     */
    float get_urgency() const;

    /**
     * Provide due date as a string
     */
    std::string due_date_string() const;

    /**
     * Get priority as string
     */
    std::string priority_string() const;
    char priority_char() const;

    // --- Upkeep ---
    /**
     * Updates the due date of a if a task is repeating, otherwise does nothing
     */
    void update_due_date();

    // --- Database functions ---

    /**
     * TODO: Synchronize task data from database into this object
     */
    int synchronize_from_db(sqlite3 *db);
};