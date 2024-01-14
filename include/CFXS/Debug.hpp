// [CFXS] //
#pragma once

#ifdef DEBUG
    #define CFXS_printf printf
    #define CFXS_ASSERT(cond, ...)                          \
        {                                                   \
            if (!(cond)) {                                  \
                CFXS_printf("Assert Failed: " __VA_ARGS__); \
                __bkpt;                                     \
            }                                               \
        }
#else
    #define CFXS_printf(...)
    #define CFXS_ASSERT(cond, ...)
#endif