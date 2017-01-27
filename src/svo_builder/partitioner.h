#pragma once

#include "../libs/libtri/include/tri_tools.h"
#include "../libs/libtri/include/trip_tools.h"
#include "../libs/libtri/include/TriReader.h"
#include "../libs/libmorton/include/morton.h"
#include "globals.h"
#include "BBoxBuffer.h"
#include "voxelizer.h"

// Partitioning-related stuff
size_t estimate_partitions(const size_t gridsize, const size_t memory_limit);
void removeTripFiles(const TripInfo &trip_info);
TripInfo partition(const TriInfo& tri_info, const size_t n_partitions, const size_t gridsize);