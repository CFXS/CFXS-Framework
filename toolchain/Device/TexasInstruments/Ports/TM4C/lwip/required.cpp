
extern "C" {

bool g_lwip_lock = false;
void lwip_check_global_lock() {
    if (g_lwip_lock) {
        asm volatile("bkpt #1");
    }
}

void lwip_process_host_timer() {
}

static uint32_t s_gr_seed_value = 1337;
uint32_t get_random_value(void) {
    s_gr_seed_value += 1337;
    return s_gr_seed_value;
}
}