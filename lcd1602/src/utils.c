#include "utils.h"

// using addr2line -e ./menu_exec <addr>

void signal_handler(int sig) {
    void *array[10];
    size_t size;

    // Получаем backtrace
    size = backtrace(array, 10);
    
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
