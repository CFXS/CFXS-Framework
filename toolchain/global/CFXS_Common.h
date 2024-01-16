// [CFXS] //
#ifndef __COMMON_PCH_HPP__
#define __COMMON_PCH_HPP__

#if defined(CFXS_ARCH_ARM)
    #if defined(CFXS_CORE_CM0) || defined(CFXS_CORE_CM0P) || defined(CFXS_CORE_CM1) || defined(CFXS_CORE_CM3) || defined(CFXS_CORE_CM4) || \
        defined(CFXS_CORE_CM7)
        #define CFXS_CORE_CORTEX_M
    #else
        #error Unknown core
    #endif

    #ifdef CFXS_CORE_CORTEX_M
        #define __bkpt asm volatile("bkpt 1") // NOLINT
    #endif
#else
    #error Unsupported architecture
#endif

#if defined(gcc) || defined(__clang__)
    #undef __used
    #undef __noinline

    #define __likely       [[likely]]
    #define __unlikely     [[unlikely]]
    #define __maybe_unused [[maybe_unused]]

    #define __interrupt        __attribute__((interrupt("irq")))
    #define __weak             __attribute__((weak))
    #define __used             __attribute__((used))
    #define __noinline         __attribute__((noinline))
    #define __noinit           __attribute__((section(".noinit")))
    #define __vector_table     __attribute__((section(".vector_table"), used))
    #define __ram_vector_table __attribute__((section(".ram_vector_table"), used))
    #define __naked            __attribute__((naked))
    #define __noreturn         __attribute__((noreturn))
    #define __noinit           __attribute__((section(".noinit")))
    #define __memory_barrier   asm volatile("" ::: "memory")
    #define __rw               volatile       // NOLINT
    #define __ro               const volatile // NOLINT

    #define __mem8(x)    (*(__rw uint8_t*)(x))  // NOLINT
    #define __mem16(x)   (*(__rw uint16_t*)(x)) // NOLINT
    #define __mem32(x)   (*(__rw uint32_t*)(x)) // NOLINT
    #define __mem64(x)   (*(__rw uint64_t*)(x)) // NOLINT
    #define __memT(T, x) (*(__rw T*)(x))        // NOLINT

    #define __nv_mem8(x)    (*(uint8_t*)(x))  // NOLINT
    #define __nv_mem16(x)   (*(uint16_t*)(x)) // NOLINT
    #define __nv_mem32(x)   (*(uint32_t*)(x)) // NOLINT
    #define __nv_mem64(x)   (*(uint64_t*)(x)) // NOLINT
    #define __nv_memT(T, x) (*(T*)(x))        // NOLINT
#else
    #error Unsupported compiler
#endif

#endif
