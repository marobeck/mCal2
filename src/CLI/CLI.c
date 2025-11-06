/**
 * Command line interface
 * ! Depreciated
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../database/database.h"
#include "log.h"

#define ISO_BUFFER_LEN 32

// Helper: Convert ISO8601 time (YYYY-MM-DDTHH:MM) to epoch
time_t parse_iso8601(const char *input)
{
    struct tm tm = {0};
    if (!strptime(input, "%Y-%m-%dT%H:%M", &tm))
    {
        LOGE("INPUT", "Invalid ISO format. Expected YYYY-MM-DDTHH:MM");
        return 0;
    }
    return mktime(&tm);
}

// Prompt and return duration in seconds
time_t prompt_duration()
{
    int hours = 0, minutes = 0;
    printf("Enter duration (hh mm): ");
    scanf("%d %d", &hours, &minutes);
    return (hours * 3600) + (minutes * 60);
}

// Prompt and return weekly flags
int prompt_weekly_flags()
{
    int flags = 0;
    char input[64];

    printf("Enter days for weekly timeblock (e.g., Mon Wed Fri):\n");
    printf("Available: Sun Mon Tue Wed Thu Fri Sat\n> ");

    getchar(); // Clear newline
    fgets(input, sizeof(input), stdin);

    char *token = strtok(input, " \n");
    while (token)
    {
        if (strcasecmp(token, "Sun") == 0)
            flags |= SUNDAY_FLAG;
        else if (strcasecmp(token, "Mon") == 0)
            flags |= MONDAY_FLAG;
        else if (strcasecmp(token, "Tue") == 0)
            flags |= TUESDAY_FLAG;
        else if (strcasecmp(token, "Wed") == 0)
            flags |= WEDNESDAY_FLAG;
        else if (strcasecmp(token, "Thu") == 0)
            flags |= THURSDAY_FLAG;
        else if (strcasecmp(token, "Fri") == 0)
            flags |= FRIDAY_FLAG;
        else if (strcasecmp(token, "Sat") == 0)
            flags |= SATURDAY_FLAG;
        else
            LOGW("INPUT", "Unknown day: %s", token);
        token = strtok(NULL, " \n");
    }

    return flags;
}

int timeblock_cli()
{
    sqlite3 *db;
    init_db(&db);

    while (1)
    {
        char name[NAME_LEN];
        char desc[DESC_LEN];
        char iso_str[ISO_BUFFER_LEN];
        int is_weekly = 0;

        printf("\n--- Create New Timeblock ---\n");
        printf("Enter name: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = 0;

        printf("Enter description: ");
        fgets(desc, sizeof(desc), stdin);
        desc[strcspn(desc, "\n")] = 0;

        printf("Is this a weekly timeblock? (1 = yes, 0 = no): ");
        scanf("%d", &is_weekly);

        time_t duration = prompt_duration();

        timeblock_t tb;
        if (is_weekly)
        {
            int flags = prompt_weekly_flags();

            int hour = 0, minute = 0;
            printf("Enter start time of day (hh mm): ");
            scanf("%d %d", &hour, &minute);
            time_t day_start = hour * 3600 + minute * 60;

            tb = create_timeblock(name, desc, flags, duration, day_start);
        }
        else
        {
            printf("Enter ISO start time (format: YYYY-MM-DDTHH:MM): ");
            scanf("%s", iso_str);

            time_t start = parse_iso8601(iso_str);
            if (start == 0)
            {
                LOGE("TIMEBLOCK", "Invalid time format. Skipping entry.\n");
                continue;
            }

            tb = create_timeblock(name, desc, 0, duration, start);
        }
        int res = db_insert_timeblock(db, &tb);
        if (res != SQLITE_OK)
        {
            LOGE("TIMEBLOCK", "Failed to save to database: %s", sqlite3_errstr(res));
        }

        LOGI("CREATED", "Timeblock '%s' (%s) created with UUID %s\n",
             tb.name, is_weekly ? "Weekly" : "Single", tb.uuid);
    }

    return 0;
}
