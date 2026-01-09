#include <cmath>
#include <cstring>

#include "log.h"
#include "database.h"
#include "task.h"

/* -------------------------------------------------------------------------- */
/*                                Constructors                                */
/* -------------------------------------------------------------------------- */

Task::Task(const char *name_, const char *desc_, Priority priority_, time_t due_date_, uint8_t frequency_)
{
    generate_uuid(uuid);
    name = strdup(name_);
    desc = strdup(desc_);

    std::memset(timeblock_uuid, 0, sizeof(timeblock_uuid));
    std::memset(completed_days, 0, sizeof(completed_days));

    due_date = due_date_;
    priority = priority_;
    status = TaskStatus::INCOMPLETE;
    goal_spec = GoalSpec::from_sql(frequency_);
    LOGI("Task::Constructor", "Created task <%s> with frequency %d", name, frequency_);
}

/* -------------------------------------------------------------------------- */
/*                                Handle habit                                */
/* -------------------------------------------------------------------------- */

static inline time_t midnight(time_t t)
{
    struct tm tm_date;
    localtime_r(&t, &tm_date);
    tm_date.tm_hour = 0;
    tm_date.tm_min = 0;
    tm_date.tm_sec = 0;
    return mktime(&tm_date);
}

void Task::update_due_date()
{
    // --- TASK MODE (non-repeating) ---
    if (status != TaskStatus::HABIT)
    {
        return;
    }

    time_t now = time(nullptr);
    time_t today_midnight = midnight(now);

    struct tm local_tm;
    localtime_r(&now, &local_tm);
    int wday = local_tm.tm_wday; // Sunday = 0

    // Check if goal has already been met today
    if (completed_days[0] == TaskStatus::COMPLETE) // Index 0 = today
    {
        due_date = 0; // clear due date
        return;
    }

    // --- HABIT MODE ---
    // Convert to flag bit
    unsigned char day_flag = 0;
    switch (wday)
    {
    case 0:
        day_flag = SUNDAY_FLAG;
        break;
    case 1:
        day_flag = MONDAY_FLAG;
        break;
    case 2:
        day_flag = TUESDAY_FLAG;
        break;
    case 3:
        day_flag = WEDNESDAY_FLAG;
        break;
    case 4:
        day_flag = THURSDAY_FLAG;
        break;
    case 5:
        day_flag = FRIDAY_FLAG;
        break;
    case 6:
        day_flag = SATURDAY_FLAG;
        break;
    }

    // --- HABIT: weekday-based ---
    if (goal_spec.mode() == GoalSpec::Mode::DayFrequency)
    {
        uint8_t day_frequency = goal_spec.weekday_flags();
        if (day_frequency & day_flag)
        {
            due_date = today_midnight; // should be done today
        }
        else
        {
            due_date = 0; // not today
        }
        return;
    }

    // --- HABIT: frequency-based (>0) ---
    int completed_this_week = 0;

    // Compute start of this week (Sunday 00:00)
    struct tm week_tm = *localtime(&now);
    week_tm.tm_hour = week_tm.tm_min = week_tm.tm_sec = 0;
    week_tm.tm_mday -= wday; // back to Sunday
    time_t week_start = mktime(&week_tm);

    // Count completions this week
    for (size_t i = 0; i < sizeof(completed_days) / sizeof(completed_days[0]); ++i)
    {
        // Current day to weekday enum
        int wday = (local_tm.tm_wday - i) % 7;

        // Only count days within this week
        struct tm day_tm = *localtime(&now);
        day_tm.tm_hour = day_tm.tm_min = day_tm.tm_sec = 0;
        day_tm.tm_mday -= i;
        time_t day_time = mktime(&day_tm);
        if (day_time < week_start)
            break;

        if (completed_days[i] == TaskStatus::COMPLETE)
        {
            ++completed_this_week;
        }
    }

    if (completed_this_week < goal_spec.frequency())
    {
        due_date = today_midnight; // should be done tonight
    }
    else
    {
        due_date = 0; // quota met, clear due date
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Interface                                 */
/* -------------------------------------------------------------------------- */

std::string Task::due_date_full_string() const
{
    if (!due_date)
    {
        return "N/A";
    }

    // Get UTC adjusted time struct
    std::time_t t = due_date;
    std::tm *tm_ptr = std::localtime(&t);

    // Return full time string
    if (!tm_ptr)
    {
        return "Invalid date";
    }

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M %p", tm_ptr);
    return std::string(buffer);
}

std::string Task::due_date_string() const
{
    if (!due_date)
    {
        return "N/A";
    }

    // Get UTC adjusted time struct
    std::time_t t = due_date;
    std::tm *tm_ptr = std::localtime(&t);

    // Return full time string
    if (!tm_ptr)
    {
        return "Invalid date";
    }

    // Return simplified time string
    time_t now = time(nullptr);
    if (t < now) // past due
    {
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_ptr);
        return std::string(buffer);
    }
    else if (difftime(t, now) < 86400) // less than one day
    {
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%I:%M %p", tm_ptr);
        return std::string(buffer);
    }
    else if (difftime(t, now) < 604800) // less than one week
    {
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%a %I:%M %p", tm_ptr);
        return std::string(buffer);
    }
    else
    {
        char buffer[16];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", tm_ptr);
        return std::string(buffer);
    }
}

std::string Task::priority_string() const
{
    switch (priority)
    {
    case Priority::NONE:
        return "None";
    case Priority::VERY_LOW:
        return "Very Low";
    case Priority::LOW:
        return "Low";
    case Priority::MEDIUM:
        return "Medium";
    case Priority::HIGH:
        return "High";
    case Priority::VERY_HIGH:
        return "Very High";
    default:
        return "Unknown";
    }
}

char Task::priority_char() const
{
    switch (priority)
    {
    case Priority::NONE:
        return 'N';
    case Priority::VERY_LOW:
        return 'V';
    case Priority::LOW:
        return 'L';
    case Priority::MEDIUM:
        return 'M';
    case Priority::HIGH:
        return 'H';
    case Priority::VERY_HIGH:
        return 'X';
    default:
        return '?';
    }
}

/* -------------------------------------------------------------------------- */
/*                                   Setters                                  */
/* -------------------------------------------------------------------------- */

void Task::set_timeblock_uuid(const char *tb_uuid)
{
    std::strncpy(timeblock_uuid, tb_uuid, UUID_LEN);
}

/* ------------------------ Get the urgency of a task ----------------------- */

/**
 * Get weight multiplier for status
 */
inline float status_weight(TaskStatus s)
{
    switch (s)
    {
    case TaskStatus::INCOMPLETE:
        return 1.0f;
    case TaskStatus::IN_PROGRESS:
        return 1.5f; // Finish what you started
    case TaskStatus::HABIT:
        return 0.8f;
    case TaskStatus::COMPLETE:
        return 0.0f;
    }
    return 0.0f;
}

/**
 * Compute deadline pressure component based on hours left until due time
 */
inline float compute_deadline_pressure(time_t now, time_t due)
{
    const float K = 7.0f;
    const float epsilon = 1.0f / 60.0f; // one minute

    std::time_t seconds_left = difftime(due, now);
    std::time_t hours_left = seconds_left / 3600.0;

    if (hours_left <= 0)
        return 100.0f; // massively urgent if overdue

    return K / std::max(static_cast<float>(hours_left), epsilon);
}

float Task::get_urgency() const
{
    // Constants
    const float K = 7.0f; // Deadline pressure constant
    const float C = 0.8f; // Undue penalty constant

    if (priority == Priority::NONE)
    {
        return 0.0f;
    }
    if (status == TaskStatus::HABIT && completed_days[0] == TaskStatus::COMPLETE)
    {
        return 0.0f;
    }

    float priority_value = static_cast<float>(priority); // Get enumerator value

    if (!due_date)
    {
        return priority_value * C * status_weight(status);
    }

    return priority_value * compute_deadline_pressure(time(nullptr), due_date) * status_weight(status);
}