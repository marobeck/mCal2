#pragma once

#include <time.h>

#include "defs.h"

typedef enum
{
    INCOMPLETE,
    IN_PROGRESS,
    COMPLETE
} TASK_STATUS;

enum class Priority
{
    NONE = -1,
    VERY_LOW = 0,
    LOW = 2,
    MEDIUM = 3,
    HIGH = 5,
    VERY_HIGH = 7
};

/** Task Struct
 * UUID string used for id to match with JSON format.
 */
class Task
{
private:
    // --- Location ---
    //? Probably unneccessary, only used for database fetching
    char uuid[UUID_LEN]; // UUID string of entry
    char timeblock_uuid[UUID_LEN];

    // --- Task Urgency ---

    time_t due_date = 0;                // Due date, 0 = undated
    Priority priority = Priority::NONE; // Priority
    TASK_STATUS status = INCOMPLETE;    // Completion status of the entry

    // --- Habit parameters ---

    /// 0 if not a habit, -1 for weekday based frequency, otherwise this indicates the number of
    /// times this habit should be done per week
    char frequency = 0;
    unsigned char day_frequency = 0;
    time_t completed_days[10]; // last 10 timestamps of days where task was complete

public:
    char *name = NULL;   // Title of entry
    char *desc = NULL;   // Verbose description of entry
    Task *prereq = NULL; // Task that must be completed prior to completing this one.

    // --- Constructors ---
    /**
     * Creates empty task
     * ! Don't use in production, will leave null pointers
     */
    Task();
    Task(const char *name_, const char *desc_, Priority priority_);

    // --- Get parameters ---
    /**
     * Get urgancy of a task
     */
    int get_urgancy();

    // --- Upkeep ---
    /**
     * Updates the due date of a if a task is repeating, otherwise does nothing
     */
    void update_due_date();
};