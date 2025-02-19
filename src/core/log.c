#include "log.h"

#include "core.h"

#include <stdio.h>
#include <stdarg.h>

static const char* levelsName[] = {
    "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
};

static const char* levelsColor[] = {
//           Foreground    Background
/* FATAL */ "\e[38;5;15m" "\e[48;5;1m",
/* ERROR */ "\e[38;5;9m",
/* WARN  */ "\e[38;5;11m",
/* INFO  */ "\e[38;5;7m",
/* DEBUG */ "\e[38;5;10m",
/* TRACE */ "\e[38;5;15m",
};

void log_msg(LogLevel level, const char* msg, ...)
{
    ASSERT(level < LOG_COUNT);

    printf("%s[%s] ", levelsColor[level], levelsName[level]);

    va_list va;
    va_start(va, msg);
    vfprintf(stdout, msg, va);
    va_end(va);

    printf("\e[0m\n");
}
