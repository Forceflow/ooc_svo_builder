#ifndef UTIL_H_
#define UTIL_H_

#include <TriMesh.h>
#include <time.h>
#include <tri_util.h>

using namespace trimesh;

// Simple timer to measure time, using CLOCKS_PER_SEC
// Should work on all platforms, sacrificing some precision.
struct Timer {
	clock_t Start;
	clock_t Elapsed;
	Timer(){
		Elapsed = 0;
		Start = clock();
	}
	void reset(){
		Start = clock();
	}
	void resetTotal(){
		Elapsed = 0;
	}
	void start(){
		Start = clock();
	}
	void stop(){
		clock_t End = clock();
		Elapsed = Elapsed + (End - Start);
	}
	double getTotalTimeSeconds() const{
		return ((double) Elapsed)/ ((double) CLOCKS_PER_SEC);
	}
};

// create bounding cube around a mesh (pad if the bbox is not a cube)
inline AABox<vec3> createMeshBBCube(const TriMesh *themesh){
	vec3 mesh_min = themesh->bbox.min;
	vec3 mesh_max = themesh->bbox.max;
	vec3 lengths = mesh_max-mesh_min;
	for(int i=0; i<3;i++){
		float delta = lengths.max() - lengths[i];
		if(delta != 0){
			mesh_min[i] = mesh_min[i] - (delta / 2.0f);
			mesh_max[i] = mesh_max[i] + (delta / 2.0f);
		}
	}
	return AABox<vec3>(mesh_min,mesh_max);
}

inline vec3 computeFaceNormal(TriMesh *themesh, size_t facenumber){
	TriMesh::Face face = themesh->faces[facenumber];
	vec3 &v0 = themesh->vertices[face[0]];
	vec3 &v1 = themesh->vertices[face[1]];
	vec3 &v2 = themesh->vertices[face[2]];
	vec3 to_normalize = (v0-v1) CROSS (v1-v2);
	return normalize(to_normalize);
}

inline vec3 getShadingFaceNormal(TriMesh *themesh, size_t facenumber){
	TriMesh::Face face = themesh->faces[facenumber];
	vec3 &n0 = themesh->normals[face[0]];
	vec3 &n1 = themesh->normals[face[1]];
	vec3 &n2 = themesh->normals[face[2]];
	vec3 to_normalize;
	to_normalize[0] = (n0[0] + n1[0] + n2[0]) / 3.0f; 
	to_normalize[1] = (n0[1] + n1[1] + n2[1]) / 3.0f; 
	to_normalize[2] = (n0[2] + n1[2] + n2[2]) / 3.0f;
	return normalize(to_normalize);
}

#endif /* UTIL_H_ */