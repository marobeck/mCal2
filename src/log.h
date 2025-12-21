#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* =========================
 *  Return Codes
 * ========================= */
#define OK                 0
#define FAIL              -1
#define ERR_NO_MEM        0x101
#define ERR_INVALID_ARG   0x102
#define ERR_INVALID_STATE 0x103
#define ERR_NOT_FOUND     0x104
#define ERR_TIMEOUT       0x105

/* =========================
 *  Logging Levels
 * ========================= */
typedef enum {
    LOG_NONE,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_VERBOSE
} log_level_t;

/* Set default log level */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

/* =========================
 *  Logging Implementation
 * ========================= */
#define LOG_FORMAT(level, tag, fmt, ...) \
    log_print(level, tag, fmt, ##__VA_ARGS__)

#define LOGE(tag, fmt, ...) LOG_FORMAT(LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) LOG_FORMAT(LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...) LOG_FORMAT(LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...) LOG_FORMAT(LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOGV(tag, fmt, ...) LOG_FORMAT(LOG_VERBOSE, tag, fmt, ##__VA_ARGS__)

static void log_print(log_level_t level, const char *tag, const char *fmt, ...) {
    if (level > LOG_LEVEL) return;

    const char *level_str = "";
    switch (level) {
        case LOG_ERROR:   level_str = "E"; break;
        case LOG_WARN:    level_str = "W"; break;
        case LOG_INFO:    level_str = "I"; break;
        case LOG_DEBUG:   level_str = "D"; break;
        case LOG_VERBOSE: level_str = "V"; break;
        default:              level_str = "?"; break;
    }

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    char timestr[9];
    if (lt)
        strftime(timestr, sizeof(timestr), "%H:%M:%S", lt);
    else
        snprintf(timestr, sizeof(timestr), "??:??:??");

    fprintf(stderr, "(%s) [%s] %s: ", timestr, level_str, tag);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

#endif // LOG_H
