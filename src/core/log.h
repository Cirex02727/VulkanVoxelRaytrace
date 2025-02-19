#ifndef LOG_H_
#define LOG_H_

typedef enum {
    LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG, LOG_TRACE, LOG_COUNT
} LogLevel;

void log_msg(LogLevel level, const char* msg, ...);

#define log_fatal(msg, ...) log_msg(LOG_FATAL, msg, ##__VA_ARGS__)
#define log_error(msg, ...) log_msg(LOG_ERROR, msg, ##__VA_ARGS__)
#define log_warn(msg, ...) log_msg(LOG_WARN, msg, ##__VA_ARGS__)
#define log_info(msg, ...) log_msg(LOG_INFO, msg, ##__VA_ARGS__)

#if defined(VKDEBUG)

    #define log_debug(msg, ...) log_msg(LOG_DEBUG, msg, ##__VA_ARGS__)
    #define log_trace(msg, ...) log_msg(LOG_TRACE, msg, ##__VA_ARGS__)

#elif defined(VKRELEASE)

    #define log_debug(msg, ...)
    #define log_trace(msg, ...)

#endif

#endif // LOG_H_
