// [CFXS] //
#pragma once

#if CFXS_DEBUG == 1
    #define CFXS_printf printf
    #define CFXS_ASSERT(cond, ...)                          \
        {                                                   \
            if (!(cond)) {                                  \
                CFXS_printf("Assert Failed: " __VA_ARGS__); \
                __bkpt;                                     \
            }                                               \
        }
    #define CFXS_BREAK(...)                     \
        {                                       \
            printf("Breakpoint: " __VA_ARGS__); \
            __bkpt;                             \
        }
    #define CFXS_ERROR(...)                \
        {                                  \
            printf("Error: " __VA_ARGS__); \
            __bkpt;                        \
        }
#else
    #define CFXS_printf(...)
    #define CFXS_ASSERT(cond, ...)
    #define CFXS_BREAK(...)
    #define CFXS_ERROR(...)
#endif