#ifndef DEFS_H
#define DEFS_H

#define DATABASE_PATH "database.db"

#define UUID_LEN 37     // 36 chars + null terminator
#define NAME_LEN 64
#define DESC_LEN 256
#define DATE_LEN 11 // YYYY-MM-DD + null

/// Priority is an offset parameter, this value decides how many seconds difference each point
/// of priority is worth.
/// Set to a quarter of a day, such that a task with a priority of 4 would be seen as having a due
/// date of 1 day sooner by the sorting algorithm.
#define PRIORITY_MULTIPLIER "21600"

// Weekly Flags
#define SATURDAY_FLAG 0x01
#define FRIDAY_FLAG 0x02
#define THURSDAY_FLAG 0x04
#define WEDNESDAY_FLAG 0x08
#define TUESDAY_FLAG 0x10
#define MONDAY_FLAG 0x20
#define SUNDAY_FLAG 0x40

#endif