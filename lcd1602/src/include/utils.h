#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <stdlib.h>

void signal_handler(int sig);

#endif // UTILS_H