extern "C" {
extern int _end;

__weak void *_sbrk(int incr) {
    return 0;
}

__weak int _close(int file) {
    return -1;
}

struct stat;
__weak int _fstat(int file, struct stat *st) {
    return -1;
}

__weak int _isatty(int file) {
    return -1;
}

__weak int _lseek(int file, int ptr, int dir) {
    return -1;
}

__weak void _exit(int status) {
}

__weak void _kill(int pid, int sig) {
    return;
}

__weak int _getpid(void) {
    return -1;
}

__weak int _write(int file, char *ptr, int len) {
    return -1;
}

__weak int _read(int file, char *ptr, int len) {
    return -1;
}

__weak void abort() {
    while (1 < 2) {
    }
}

extern "C" int __aeabi_atexit(void *obj, void (*dtr)(void *), void *dso_h) {
    (void)obj;
    (void)dtr;
    (void)dso_h;
    return 0;
}

void *__dso_handle = 0;

/**
 * This is an error handler that is invoked by the C++ runtime when a pure virtual function is called.
 * If anywhere in the runtime of your program an object is created with a virtual function pointer not
 * filled in, and when the corresponding function is called, you will be calling a 'pure virtual function'.
 * The handler you describe should be defined in the default libraries that come with your development environment.
 */
void __cxa_pure_virtual() {
    while (1 < 2) {
    }
}
}

namespace __gnu_cxx {

    void __verbose_terminate_handler() {
        while (1 < 2) {
        }
    }

} // namespace __gnu_cxx