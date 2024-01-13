#include <SEGGER_RTT.h>

__weak void __cfxs_putchar(int c) {
    SEGGER_RTT_Write(0, static_cast<char*>(static_cast<void*>(&c)), 1);
}
