#pragma once

#include "svo_builder_util.h"

using namespace std;

// global flag: be verbose about what we do?
extern bool verbose;

// Timers (for debugging purposes)
// (This is a bit ugly, but it's a quick and surefire way to measure performance)

// Main program timer
extern Timer main_timer;

// Timers for partitioning step
extern Timer part_total_timer;
extern Timer part_io_in_timer;
extern Timer part_io_out_timer;
extern Timer part_algo_timer;

// Timers for voxelizing step
extern Timer vox_total_timer;
extern Timer vox_io_in_timer;
extern Timer vox_algo_timer;

// Timers for SVO building step
extern Timer svo_total_timer;
extern Timer svo_io_out_timer;
extern Timer svo_algo_timer;