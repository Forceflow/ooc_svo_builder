#include <TriMesh.h>
#include <stdint.h>

using namespace trimesh;
using namespace std;

// This struct defines VoxelData for our voxelizer - this is defined different depending on compile project
// This is the main memory hogger: the less data you store here, the better.
struct VoxelData{
	uint64_t morton;
	vec3 normal;
	vec3 color;

	VoxelData() : morton(0), normal(vec3(0.0f, 0.0f, 0.0f)), color(vec3(0.0f, 0.0f, 0.0f)){}
	VoxelData(uint64_t morton, vec3 normal, vec3 color) : morton(morton), normal(normal), color(color){}

	bool operator >(VoxelData &a){
		return morton > a.morton;
	}

	bool operator <(VoxelData &a){
		return morton < a.morton;
	}
};