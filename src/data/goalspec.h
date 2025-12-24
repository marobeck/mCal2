#pragma once

#include <cstdint>

enum class Weekday : uint8_t
{
    Saturday = 0x01,
    Friday = 0x02,
    Thursday = 0x04,
    Wednesday = 0x08,
    Tuesday = 0x10,
    Monday = 0x20,
    Sunday = 0x40
};

class GoalSpec
{
public:
    enum class Mode : uint8_t
    {
        DayFrequency = 0,
        Frequency = 1
    };

private:
    static constexpr uint8_t MODE_BIT = 0x80;
    static constexpr uint8_t PAYLOAD_MASK = 0x7F;

public:
    uint8_t data{0};

    /* ---- Constructors ---- */

    // Day-frequency constructor
    static GoalSpec day_frequency(uint8_t weekday_flags)
    {
        GoalSpec g;
        g.data = weekday_flags & PAYLOAD_MASK; // mode bit = 0
        return g;
    }

    // Frequency constructor
    static GoalSpec frequency(uint8_t times_per_week)
    {
        GoalSpec g;
        g.data = MODE_BIT | (times_per_week & PAYLOAD_MASK);
        return g;
    }

    /* ---- Queries ---- */

    Mode mode() const
    {
        return (data & MODE_BIT) ? Mode::Frequency : Mode::DayFrequency;
    }

    uint8_t frequency() const
    {
        return data & PAYLOAD_MASK;
    }

    uint8_t weekday_flags() const
    {
        return data & PAYLOAD_MASK;
    }

    // Check if a given DayFrequency weekday is included
    bool has_day(Weekday d) const
    {
        return (weekday_flags() & static_cast<uint8_t>(d)) != 0;
    }

    // Check if a given tm_wday (0 = Sunday, 6 = Saturday) is included
    bool has_day(int tm_wday) const
    {
        int weekday_flag = 1 << (6 - tm_wday); // Map weekday to flag mask
        return (weekday_flags() & weekday_flag) != 0;
    }

    bool is_empty() const
    {
        return (data & PAYLOAD_MASK) == 0;
    }

    /* ---- SQL Serialization ---- */

    uint8_t to_sql() const
    {
        return data;
    }

    static GoalSpec from_sql(uint8_t value)
    {
        GoalSpec g;
        g.data = value;
        return g;
    }
};
