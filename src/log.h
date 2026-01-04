#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

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

    /* Get time with millisecond precision */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t sec = tv.tv_sec;
    struct tm lt_buf;
    struct tm *lt = localtime_r(&sec, &lt_buf);
    char timestr[16];
    if (lt) {
        char tmp[12];
        strftime(tmp, sizeof(tmp), "%H:%M:%S", lt);
        int ms = (int)(tv.tv_usec / 1000);
        snprintf(timestr, sizeof(timestr), "%s.%03d", tmp, ms);
    } else {
        snprintf(timestr, sizeof(timestr), "??:??:??.???");
    }

    /* Colorize output when stderr is a TTY */
    int use_color = isatty(fileno(stderr));
    const char *col_start = "";
    const char *col_end = "";
    if (use_color) {
        switch (level) {
            case LOG_ERROR:   col_start = "\x1b[31m"; break; /* red */
            case LOG_WARN:    col_start = "\x1b[33m"; break; /* yellow */
            case LOG_INFO:    col_start = "\x1b[32m"; break; /* green */
            case LOG_DEBUG:   col_start = "\x1b[36m"; break; /* cyan */
            case LOG_VERBOSE: col_start = "\x1b[35m"; break; /* magenta */
            default:          col_start = ""; break;
        }
        col_end = "\x1b[0m";
    }

    fprintf(stderr, "%s(%s) [%s] %s: ", col_start, timestr, level_str, tag);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "%s\n", col_end);
}

#endif // LOG_H
