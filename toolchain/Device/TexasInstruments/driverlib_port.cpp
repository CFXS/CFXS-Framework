#include <inc/hw_ints.h>
#include <driverlib/interrupt.h>
#include <driverlib/cpu.h>
#include <CFXS/CPU.hpp>
#include <CFXS/Utils.hpp>

extern "C" {

uint32_t CPUcpsid() {
    return !CFXS::CPU::check_and_disable_interrupts();
}

uint32_t CPUcpsie() {
    CFXS::CPU::enable_interrupts();
    return 1;
}

void IntRegister(uint32_t number, CFXS::VoidFunction handler) { // NOLINT
    CFXS::CPU::set_interrupt_handler(number, handler);
}

void IntUnregister(uint32_t number) { // NOLINT
    CFXS::CPU::set_interrupt_handler(number, nullptr);
}
}
