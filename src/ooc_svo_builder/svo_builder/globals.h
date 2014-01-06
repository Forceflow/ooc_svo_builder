#ifndef GLOBALS_H_
#define GLOBALS_H_

#include "svo_builder_util.h"

using namespace std;

// Timers (for debugging purposes)
// (This is a bit ugly, but it's a quick and surefire way to measure performance)
extern Timer main_timer;

extern Timer part_total_timer;
extern Timer part_io_in_timer;
extern Timer part_io_out_timer;
extern Timer part_algo_timer;

extern Timer vox_total_timer;
extern Timer vox_io_in_timer;
extern Timer vox_algo_timer;

extern Timer svo_total_timer;
extern Timer svo_io_out_timer;
extern Timer svo_algo_timer;

// global flag: be verbose about what we do?
extern bool verbose;

#endif // GLOBALS_H_