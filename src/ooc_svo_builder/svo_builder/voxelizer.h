#ifndef VOXELIZER_H_
#define VOXELIZER_H_

#include "globals.h"
#include "intersection.h"
#include "morton.h"
#include <tri_tools.h>
#include <TriReader.h>

// Voxelization-related stuff

using namespace std;

#ifdef BINARY_VOXELIZATION
struct VoxelData{
	bool filled;
	VoxelData(): filled(false){}
	VoxelData(bool filled): filled(filled){}
};
#else
struct VoxelData{
	bool filled;
	vec3 normal;
	VoxelData(): filled(false),normal(vec3(0.0f,0.0f,0.0f)){}
	VoxelData(bool filled, vec3 normal): filled(filled),normal(normal){}
};
#endif

void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, float unitlength, VoxelData** data, size_t &nfilled);

#endif // VOXELIZER_H_