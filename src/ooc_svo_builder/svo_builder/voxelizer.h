#ifndef VOXELIZER_H_
#define VOXELIZER_H_

#include <tri_tools.h>
#include <TriReader.h>
#include "globals.h"
#include "intersection.h"
#include "morton.h"
#include "VoxelData.h"

// Voxelization-related stuff
typedef Vec<3, unsigned int> uivec3;
using namespace std;

#define EMPTY_VOXEL 0
#define FULL_VOXEL 1

#ifdef BINARY_VOXELIZATION
void voxelize_huang_method(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled);
#else
void voxelize_huang_method(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled);
#endif

#ifdef BINARY_VOXELIZATION
void voxelize_schwarz_method(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, char* voxels, vector<uint64_t> &data, float sparseness_limit, bool &use_data, size_t &nfilled);
#else
void voxelize_schwarz_method(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, char* voxels, vector<VoxelData> &data, float sparseness_limit, bool &use_data, size_t &nfilled);
#endif

//#ifdef BINARY_VOXELIZATION
//void voxelize_partition3(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, char* voxels, vector<uint64_t> &data, float sparseness_limit, bool &use_data, size_t &nfilled);
//#else
//void voxelize_partition3(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, char* voxels, vector<uint64_t> &data, float sparseness_limit, bool &use_data, size_t &nfilled);
//#endif

#endif // VOXELIZER_H_