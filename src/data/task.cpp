#include "task.h"

#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>

/* -------------------------------------------------------------------------- */
/*                                Constructors                                */
/* -------------------------------------------------------------------------- */

Task::Task()
{
    name = strdup("");
    desc = strdup("");

    std::memset(uuid, 0, sizeof(uuid));
    std::memset(timeblock_uuid, 0, sizeof(timeblock_uuid));
    std::memset(completed_days, 0, sizeof(completed_days));

    due_date = 0;
    priority = Priority::NONE;
    status = INCOMPLETE;
    frequency = 0;
    day_frequency = 0;
}

Task::Task(const char *name_, const char *desc_, Priority priority_, time_t due_date_, char frequency_, unsigned char day_frequency_)
{
    name = strdup(name_);
    desc = strdup(desc_);

    std::memset(uuid, 0, sizeof(uuid));
    std::memset(timeblock_uuid, 0, sizeof(timeblock_uuid));
    std::memset(completed_days, 0, sizeof(completed_days));

    due_date = due_date_;
    priority = priority_;
    status = INCOMPLETE;
    frequency = frequency_;
    day_frequency = day_frequency_;
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
    if (frequency == 0)
    {
        return;
    }

    // --- HABIT MODE ---
    time_t now = time(nullptr);
    time_t today_midnight = midnight(now);

    struct tm local_tm;
    localtime_r(&now, &local_tm);
    int wday = local_tm.tm_wday; // Sunday = 0

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

    // --- HABIT: weekday-based (-1) ---
    if (frequency == -1)
    {
        if (day_frequency & day_flag)
            due_date = today_midnight; // should be done today
        else
            due_date = 0; // not today
        return;
    }

    // --- HABIT: frequency-based (>0) ---
    int completed_this_week = 0;

    // Compute start of this week (Sunday 00:00)
    struct tm week_tm = *localtime(&now);
    week_tm.tm_hour = week_tm.tm_min = week_tm.tm_sec = 0;
    week_tm.tm_mday -= wday; // back to Sunday
    time_t week_start = mktime(&week_tm);

    for (time_t t : completed_days)
    {
        if (t >= week_start)
            completed_this_week++;
    }

    if (completed_this_week < frequency)
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
    // TODO: String should be modified based on time left (i.e. year should be excluded if it's the same year as is due)
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %I:%M:%S %p");
    return oss.str(); // Return by value
}

int Task::get_urgancy() const
{
    if (priority == Priority::NONE)
    {
        return 0;
    }

    if (!due_date)
    {
        return static_cast<int>(priority); // Get enumerator value
    }

    // time remaining * -1
    std::time_t exp = std::time(nullptr) - due_date;

    return pow(2, exp) + static_cast<int>(priority);
}