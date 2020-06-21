#ifndef LOG_H
#define LOG_H

enum LogLevel { LOG_DEBUG = 0, LOG_INFO = 1, LOG_WARN = 2, LOG_ERROR = 3 };
extern enum LogLevel LOGGER_LEVEL;

void die_err(const char *msg);
void die_msg(const char *msg);

void increase_verbosity();
void decrease_verbosity();

void log_msg  (enum LogLevel level, const char *fmt, ...);
void log_errno(enum LogLevel level, const char *fmt, ...);

#endif /* LOG_H */
