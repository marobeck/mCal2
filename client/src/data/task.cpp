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
    generate_uuid(uuid.value);
    name = strdup(name_);
    desc = strdup(desc_);

    std::memset(timeblock_uuid.value, 0, sizeof(timeblock_uuid.value));
    std::memset(completed_days, 0, sizeof(completed_days));

    due_date = due_date_;
    priority = priority_;
    status = TaskStatus::INCOMPLETE;
    completed_datetime = 0;
    goal_spec = GoalSpec::from_sql(frequency_);
    LOGI("Task::Constructor", "Created task <%s> with frequency %d", name, frequency_);
}

// Copy constructor: deep-copy heap strings and other fields
Task::Task(const Task &other)
{
    uuid = other.uuid;
    timeblock_uuid = other.timeblock_uuid;
    due_date = other.due_date;
    priority = other.priority;
    scope = other.scope;
    status = other.status;
    completed_datetime = other.completed_datetime;
    prerequisites = other.prerequisites;
    goal_spec = other.goal_spec;
    std::memcpy(completed_days, other.completed_days, sizeof(completed_days));
    name = other.name ? strdup(other.name) : nullptr;
    desc = other.desc ? strdup(other.desc) : nullptr;
}

// Copy assignment operator
Task &Task::operator=(const Task &other)
{
    if (this != &other)
    {
        free(name);
        free(desc);
        uuid = other.uuid;
        timeblock_uuid = other.timeblock_uuid;
        due_date = other.due_date;
        priority = other.priority;
        scope = other.scope;
        status = other.status;
        completed_datetime = other.completed_datetime;
        prerequisites = other.prerequisites;
        goal_spec = other.goal_spec;
        std::memcpy(completed_days, other.completed_days, sizeof(completed_days));
        name = other.name ? strdup(other.name) : nullptr;
        desc = other.desc ? strdup(other.desc) : nullptr;
    }
    return *this;
}

Task::~Task()
{
    free(name);
    free(desc);
    // Prerequisites are not owned by Task, so we don't free them either
}

/* -------------------------------------------------------------------------- */
/*                                Handle habit                                */
/* -------------------------------------------------------------------------- */

// The end of day time for a given time t
static inline time_t midnight(time_t t)
{
    struct tm tm_date;
    localtime_r(&t, &tm_date);
    tm_date.tm_hour = 23;
    tm_date.tm_min = 59;
    tm_date.tm_sec = 59;
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
        day_flag = static_cast<unsigned char>(Weekday::Sunday);
        break;
    case 1:
        day_flag = static_cast<unsigned char>(Weekday::Monday);
        break;
    case 2:
        day_flag = static_cast<unsigned char>(Weekday::Tuesday);
        break;
    case 3:
        day_flag = static_cast<unsigned char>(Weekday::Wednesday);
        break;
    case 4:
        day_flag = static_cast<unsigned char>(Weekday::Thursday);
        break;
    case 5:
        day_flag = static_cast<unsigned char>(Weekday::Friday);
        break;
    case 6:
        day_flag = static_cast<unsigned char>(Weekday::Saturday);
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

std::string Task::scope_string() const
{
    switch (scope)
    {
    case Scope::XS:
        return "XS";
    case Scope::S:
        return "S";
    case Scope::M:
        return "M";
    case Scope::L:
        return "L";
    case Scope::XL:
        return "XL";
    default:
        return "Unknown";
    }
}

char Task::scope_char() const
{
    switch (scope)
    {
    case Scope::XS:
        return 's';
    case Scope::S:
        return 'S';
    case Scope::M:
        return 'M';
    case Scope::L:
        return 'L';
    case Scope::XL:
        return 'X';
    default:
        return '?';
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
    std::strncpy(timeblock_uuid.value, tb_uuid, UUID_LEN);
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
    /**
     * Pressure function: pressure = exp(-T, t)
     * T = time until deadline in hours
     * t = time constant (half life of pressure in hours)
     */
    const float t = 48.0f; // Half life of 48 hours; TODO: make this user-configurable
    float T = difftime(due, now) / 3600.0f;

    // Linear scaling for overdue tasks to avoid very large pressures
    if (T < 0)
    {
        return std::max(std::abs(T), 1.0f);
    }

    return std::exp(-T / t);
}

float Task::get_urgency() const
{
    const char *TAG = "Task::get_urgency";

    // Constants
    const float C = g_score_weights.undated_pressure_constant; // Undated pressure constant
    const float w_p = g_score_weights.priority_weight;         // Priority weight
    const float w_u = g_score_weights.due_date_weight;         // Urgency/deadline pressure weight
    const float w_e = g_score_weights.scope_weight;            // Effort/scope weight

    // Execptions
    if (priority == Priority::NONE)
    {
        return 0.0f;
    }
    if (status == TaskStatus::HABIT && completed_days[0] == TaskStatus::COMPLETE)
    {
        return 0.0f;
    }

    // Urgency is negative if blocked by an incomplete prerequisite
    bool blocked = false;
    for (const Task *prereq : prerequisites)
    {
        if (prereq->status != TaskStatus::COMPLETE)
        {
            blocked = true;
            break;
        }
    }

    // --- Calculate urgency ---

    /** Eat the frog
      score_frog = w_u * U
      + w_p * P_norm
      + w_e * E_norm
     */

    // Normalize priority and effort to [0, 1]
    double P_norm = (static_cast<int>(priority) - static_cast<int>(Priority::VERY_LOW)) / static_cast<int>(Priority::VERY_HIGH);
    double E_norm = (static_cast<int>(scope) - static_cast<int>(Scope::XS)) / static_cast<int>(Scope::XL);

    // Compute deadline pressure, from [0, 1] or [1, inf) if overdue, or constant C if undated
    double pressure = due_date ? compute_deadline_pressure(time(nullptr), due_date) : C;

    float intrinsic_urgency = (w_u * pressure + w_p * P_norm + w_e * E_norm) * status_weight(status);

    // Inherted urgency
    float max_inherited_urgency = 0.0f;
    for (const Task *dep : dependents)
    {
        if (!dep)
        {
            LOGW(TAG, "Skipping invalid dependency pointer");
        }
        // Use abs() because a dependent might be blocked (returning negative),
        // but we still need its true potential urgency.
        float dep_urgency = std::abs(dep->get_urgency());
        if (dep_urgency > max_inherited_urgency)
        {
            max_inherited_urgency = dep_urgency;
        }
    }

    float true_urgency = std::max(intrinsic_urgency, max_inherited_urgency);

    return blocked ? -true_urgency : true_urgency;
}