#ifndef GLOBALS_H_
#define GLOBALS_H_

// global header containing timers (this is a bit ugly, but it's a quick and surefire way to measure performance)
#include <fstream>
#include <iostream>
#include "svo_builder_util.h"

using namespace std;

// timers
extern Timer main_timer;
extern Timer algo_timer;
extern Timer io_timer_in;
extern Timer io_timer_out;

// global flag: be verbose about what we do?
extern bool verbose;

#endif // GLOBALS_H_