// [CFXS] //
#pragma once

namespace CFXS::CPU {

    // Core reset
    __noreturn void reset();

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
    // Check if global interrupts are enabled
    bool are_interrupts_enabled();

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
        NoInterruptScope() : m_interrupts_were_enabled(are_interrupts_enabled()) {
            disable_interrupts();
        }
        ~NoInterruptScope() {
            if (m_interrupts_were_enabled)
                enable_interrupts();
        }

    private:
        bool m_interrupts_were_enabled;
    };

} // namespace CFXS::CPU