#include "CPU.hpp"

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
        asm volatile("cpsie i" ::: "memory");
    }
    void disable_interrupts() {
        asm volatile("cpsid i" ::: "memory");
    }
    bool are_interrupts_enabled() {
        return !get_primask();
    }

} // namespace CFXS::CPU