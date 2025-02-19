#ifndef CORE_H_
#define CORE_H_

#include "list_types.h"
#include "log.h"

#include <stdbool.h>
#include <stdio.h>

#if defined(VKDEBUG)

    #define ASSERT(x) { if(!(x)) __builtin_trap(); }
    #define ASSERT_MSG(x, msg) { if(!(x)) { printf("%s\n", msg); __builtin_trap(); } }

    #define IFDEBUG(x) x
    #define IFRELEASE(x)

#elif defined(VKRELEASE)

    #define ASSERT(x)
    #define ASSERT_MSG(x, msg)
    
    #define IFDEBUG(x)
    #define IFRELEASE(x) x

#endif

#define UNUSED(x) ((void)x)

#define ARRAYLEN(x) (sizeof(x) / sizeof(*(x)))

#define finalize(x) { result = (x); goto finalize; }

int time_to_str(char* str, double bytes);
int num_to_str(char* str, double bytes);

#endif // CORE_H_
