#pragma once

#include <ctime>
#include <string>
#include <sqlite3.h>

#include "defs.h"
#include "goalspec.h"

enum class Priority
{
    NONE = -1,
    VERY_LOW = 1,
    LOW = 3,
    MEDIUM = 4,
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

    GoalSpec goal_spec; // Goal specification for habit tasks

    // The last days where task was complete (for GUI preview) where index 0 = today
    // INCOMPLETE = not done, COMPLETE = done, IN_PROGRESS = to do
    TaskStatus completed_days[10];

    // --- Descriptive fields ---
    char *name = NULL;   // Title of entry
    char *desc = NULL;   // Verbose description of entry
    Task *prereq = NULL; // Task that must be completed prior to completing this one.

    // --- Constructors ---

    Task() = default;
    Task(const char *name_, const char *desc_, Priority priority_ = Priority::NONE, time_t due_date_ = 0, uint8_t frequency_ = 0);

    // --- Setters ---

    void set_timeblock_uuid(const char *tb_uuid);

    // --- Get parameters ---

    /**
     * Get urgency of a task
     */
    float get_urgency() const;

    /**
     * Provide due date as a string
     */
    std::string due_date_full_string() const;
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
};