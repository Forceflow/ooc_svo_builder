#ifndef VOXELIZER_H_
#define VOXELIZER_H_

#include "globals.h"
#include "intersection.h"
#include "morton.h"
#include <tri_tools.h>
#include <TriReader.h>

// Voxelization-related stuff

using namespace std;
// This struct defines VoxelData for our voxelizer - this is defined different depending on compile project
// This is the main memory hogger: the less data you store here, the better.
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
	vec3 color;
	VoxelData(): filled(false), normal(vec3(0.0f,0.0f,0.0f)), color(vec3(0.0f,0.0f,0.0f)){}
	VoxelData(bool filled, vec3 normal, vec3 color): filled(filled), normal(normal), color(color){}
};
#endif

void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, VoxelData** data, size_t &nfilled);

#endif // VOXELIZER_H_