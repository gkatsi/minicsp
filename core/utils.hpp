#ifndef __MINICSP_UTILS_HPP__
#define __MINICSP_UTILS_HPP__

#include <stdint.h>

class Solver;

double cpuTime(void);
uint64_t memUsed();
void printStats(Solver& solver, const char *comment = 0L);
void setup_signal_handlers(Solver *s);


#endif

