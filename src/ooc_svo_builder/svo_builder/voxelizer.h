#ifndef VOXELIZER_H_
#define VOXELIZER_H_

#include "globals.h"
#include "intersection.h"
#include "morton.h"
#include <tri_tools.h>
#include <TriReader.h>

// Voxelization-related stuff

typedef Vec<3, unsigned int> uivec3;

using namespace std;
// This struct defines VoxelData for our voxelizer - this is defined different depending on compile project
// This is the main memory hogger: the less data you store here, the better.
struct VoxelData{
	vec3 normal;
	vec3 color;
	VoxelData(): normal(vec3(0.0f,0.0f,0.0f)), color(vec3(0.0f,0.0f,0.0f)){}
	VoxelData(vec3 normal, vec3 color): normal(normal), color(color){}
};

#ifdef BINARY_VOXELIZATION
#define EMPTY_VOXEL false // false means no voxel
void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled);
#else
#define EMPTY_VOXEL 0 // false means no voxel
void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled);
#endif

#ifdef BINARY_VOXELIZATION
void voxelize_partition2(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled);
#else
void voxelize_partition2(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled);
#endif

#ifdef BINARY_VOXELIZATION
void voxelize_partition3(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled);
#else
void voxelize_partition3(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled);
#endif

#endif // VOXELIZER_H_