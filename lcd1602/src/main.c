#include "lcd1602_sdl.h"
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>

void signal_handler(int sig) {
    void *array[10];
    size_t size;

    // Получаем backtrace
    size = backtrace(array, 10);
    
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(void)
{
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);

    sdl_handle_t *sdl = lcd1602_sdl_create("LCD1602", 100, 100);

    while (lcd1602_sdl_next_tick(sdl)) {
        SDL_Delay(100);
    }

    lcd1602_sdl_release(sdl);

    return 0;
}