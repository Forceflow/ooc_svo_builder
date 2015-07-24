#ifndef PARTITIONER_H_
#define PARTITIONER_H_

#include "../libs/tri_tools/include/tri_tools.h"
#include "../libs/tri_tools/include/trip_tools.h"
#include "../libs/tri_tools/include/file_tools.h""
#include "globals.h"
#include <TriReader.h>
#include "Buffer.h"
#include "morton.h"
#include "voxelizer.h"

// Partitioning-related stuff
size_t estimate_partitions(const size_t gridsize, const size_t memory_limit);
void removeTripFiles(const TripInfo &trip_info);
TripInfo partition(const TriInfo& tri_info, const size_t n_partitions, const size_t gridsize);

#endif /* PARTITIONER_H_ */