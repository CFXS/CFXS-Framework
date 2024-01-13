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
}