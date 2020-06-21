#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void die_err(const char *msg)
{
    printf("ERROR: %s: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void die_msg(const char *msg)
{
    printf("ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}

void print_log_prefix(enum LogLevel level)
{
    switch (level)
    {
    case LOG_DEBUG:
        printf("[DEBUG] ");
        break;
    case LOG_INFO:
        printf("[INFO] ");
        break;
    case LOG_WARN:
        printf("[WARN] ");
        break;
    case LOG_ERROR:
        printf("[ERROR] ");
        break;
    }
}

void log_msg(enum LogLevel level, const char *fmt, ...)
{
    if (level < LOGGER_LEVEL) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    print_log_prefix(level);
    vprintf(fmt, args);
    putchar('\n');

    va_end(args);
}

void log_errno(enum LogLevel level, const char *fmt, ...)
{
    if (level < LOGGER_LEVEL) {
        return;
    }

    char *error_sring = strerror(errno);

    va_list args;
    va_start(args, fmt);

    print_log_prefix(level);
    vprintf(fmt, args);
    puts(error_sring);

    va_end(args);
}

void increase_verbosity()
{
    if (LOGGER_LEVEL > LOG_DEBUG) {
        LOGGER_LEVEL--;
    }
}

void decrease_verbosity()
{
    if (LOGGER_LEVEL < LOG_ERROR) {
        LOGGER_LEVEL++;
    }
}

