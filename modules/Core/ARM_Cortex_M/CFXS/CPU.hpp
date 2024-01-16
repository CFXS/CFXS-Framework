// [CFXS] //
#pragma once
#include <CFXS/Utils.hpp>

namespace CFXS::CPU {

    // Core reset
    __noreturn void reset();

    // Set system timer callback
    void set_system_timer(VoidFunction handler, uint32_t period);

    // Check if debug system is enabled
    bool is_debug_enabled();
    // Enable cycle counter
    void enable_cycle_counter();
    // Get current value of cycle counter
    uint32_t get_cycle_counter();

    // Enable global interrupts
    void enable_interrupts();
    // Disable global interrupts
    void disable_interrupts();
    // Disable global interrupts and return true if were enabled
    bool check_and_disable_interrupts();
    // Check if global interrupts are enabled
    bool are_interrupts_enabled();

    // Set interrupt handler and move VTOR to RAM if not moved already
    void set_interrupt_handler(uint32_t number, VoidFunction handler);

    /// Wait for n cycles (precision increment: 4 cycles)
    void delay(uint32_t cycles);

    /// Read IPSR register
    inline uint32_t get_ipsr() {
        uint32_t val;
        asm volatile("mrs %0, ipsr" : "=r"(val));
        return val;
    }

    /// Read PRIMASK register
    inline uint32_t get_primask() {
        uint32_t val;
        asm volatile("mrs %0, primask" : "=r"(val));
        return val;
    }

    /// Read BASEPRI register
    inline uint32_t get_basepri() {
        uint32_t val;
        asm volatile("mrs %0, basepri" : "=r"(val));
        return val;
    }

    /// Read SP register
    inline uint32_t get_sp() {
        uint32_t val;
        asm volatile("mrs %0, msp" : "=r"(val));
        return val;
    }

    /// Read LR register
    inline uint32_t get_lr() {
        uint32_t val;
        asm volatile("mov %0, lr" : "=r"(val));
        return val;
    }

    /// Read PC register
    inline uint32_t get_pc() {
        uint32_t val;
        asm volatile("mov %0, pc" : "=r"(val));
        return val;
    }

    class NoInterruptScope {
    public:
        NoInterruptScope() : m_interrupts_were_enabled(check_and_disable_interrupts()) {
        }
        ~NoInterruptScope() {
            if (m_interrupts_were_enabled)
                enable_interrupts();
        }

    private:
        bool m_interrupts_were_enabled;
    };

} // namespace CFXS::CPU