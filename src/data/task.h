#pragma once

#include <ctime>
#include <string>
#include <sqlite3.h>

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
    // Used for database fetching
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

    /**
     * Creates task
     */
    Task(const char *name_, const char *desc_, Priority priority_ = Priority::NONE, time_t due_date_ = 0, char frequency_ = 0, unsigned char day_frequency_ = 0);

    // --- Get parameters ---
    /**
     * Get urgency of a task
     */
    float get_urgency() const;

    /**
     * Provide due date as a string
     */
    std::string due_date_string() const;

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