#ifndef PARTITIONER_H_
#define PARTITIONER_H_

#include "Buffer.h"
#include "morton.h"
#include <tri_tools.h>
#include <trip_tools.h>
#include <TriReader.h>
#include "voxelizer.h"

// Partitioning-related stuff

size_t estimate_partitions(const size_t gridsize, const size_t memory_limit);
void removeTripFiles(TripInfo &trip_info);
TripInfo partition(TriInfo& tri_info, size_t n_partitions, size_t gridsize);

#endif /* PARTITIONER_H_ */