#include "CPU.hpp"
#include <sys/cdefs.h>
#include "CFXS/Utils.hpp"

namespace CFXS::CPU {

    __noreturn void reset() {
        // [0xE000ED0C] Application Interrupt and Reset Control
        // [0x05FA0000] Vector Key
        // [0x00000004] System Reset Request
        __mem32(0xE000ED0C) = 0x05FA0000 | 0x00000004;
        while (1 < 2) {
        }
    }

    bool is_debug_enabled() {
        // [0xE000EDF0] Debug Control and Status Register
        return __mem32(0xE000EDF0) & 1;
    }
    void enable_cycle_counter() {
        // [0xE0001000] DWT_CTRL
        // [Bit 1] DWT_CTRL_CYCNTENA
        __mem32(0xE0001000) |= 1;
    }

    uint32_t get_cycle_counter() {
        // [0xE0001004] DWT_CYCCNT
        return __mem32(0xE0001004);
    }

    void enable_interrupts() {
        asm volatile("cpsie i");
    }
    void disable_interrupts() {
        asm volatile("cpsid i");
    }
    __naked bool check_and_disable_interrupts() {
        asm volatile("    mrs     r0, PRIMASK\n"
                     "    cpsid   i          \n"
                     "    eor     r0, #1     \n"
                     "    bx      lr\n");
    }
    bool are_interrupts_enabled() {
        return !get_primask();
    }

    __naked void delay(__maybe_unused uint32_t cycles) {
        asm volatile("    lsr  r0, #2           \n" // divide cycles by 4
                     "__cfxs_cpu_delay:         \n"
                     "    nop                   \n" // Extend this loop to 4 cycles
                     "    subs r0, #1           \n" // subtract 1
                     "    bne  __cfxs_cpu_delay \n" // repeat if > 0
                     "    bx   lr                 " // return
        );
    }

    void set_system_timer(VoidFunction handler, uint32_t period) {
        set_interrupt_handler(15, handler); // [15] SysTick
        // [0xE000E010] SysTick Control and Status Register
        // [0xE000E014] SysTick Reload Value Register
        __mem32(0xE000E014) = period - 1;
        __mem32(0xE000E010) |= 0x01 | 0x02 | 0x04; // enable clock, interrupt, systick
    }

// TODO: find good way to optimize interrupt count - where to define the value in config
#ifndef CFXS_CPU_INTERRUPT_COUNT
    #define CFXS_CPU_INTERRUPT_COUNT 256
#endif
    __ram_vector_table VoidFunction g_vector_table[CFXS_CPU_INTERRUPT_COUNT];
    void set_interrupt_handler(__maybe_unused uint32_t number, __maybe_unused VoidFunction handler) {
        NoInterruptScope _;
        // [0xE000ED08] Vector Table Offset Register
        const auto vtor = __mem32(0xE000ED08);
        if (vtor != reinterpret_cast<uint32_t>(g_vector_table)) {
            memcpy(g_vector_table, reinterpret_cast<void*>(vtor), sizeof(g_vector_table));
            // Set VTOR to local table
            __mem32(0xE000ED08) = reinterpret_cast<uint32_t>(g_vector_table);
        }
        g_vector_table[number] = handler;
    }

} // namespace CFXS::CPU