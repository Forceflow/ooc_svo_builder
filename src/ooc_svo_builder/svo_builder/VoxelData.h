#ifndef VOXELDATA_H_
#define VOXELDATA_H_

#include <TriMesh.h>
#include <stdint.h>

using namespace trimesh;
using namespace std;

// sizeof(VoxelData would work too)
const size_t VOXELDATA_SIZE = sizeof(uint64_t)+2 * (3 * sizeof(float));

// This struct defines VoxelData for our voxelizer.
// This is the main memory hogger: the less data you store here, the better.
struct VoxelData{
	uint64_t morton;
	vec3 color;
	vec3 normal;
	
	VoxelData() : morton(0), normal(vec3()), color(vec3()){}
	VoxelData(uint64_t morton, vec3 normal, vec3 color) : morton(morton), normal(normal), color(color){}

	bool operator > (const VoxelData &a) const{
		return morton > a.morton;
	}

	bool operator < (const VoxelData &a) const{
		return morton < a.morton;
	}
};

#endif // VOXELDATA_H_