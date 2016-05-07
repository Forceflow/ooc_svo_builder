#pragma once

#include <TriMesh.h>
#include <glm/glm.hpp>
#include "timer.h"
#include "../libs/libtri/include/tri_util.h"
#include "../libs/libtri/include/tri_tools.h"

// convert between trimesh::vec3 and glm::vec3
inline glm::vec3 toGLM(trimesh::vec3 v) {
	return glm::vec3(v[0], v[1], v[2]);
}

// convert between glm::vec3 and trimesh::vec3
inline trimesh::vec3 toTriMesh(glm::vec3 v) {
	return trimesh::vec3(v[0], v[1], v[2]);
}

// create bounding cube around a mesh (pad if the bbox is not a cube)
inline AABox<glm::vec3> createMeshBBCube(const trimesh::TriMesh *themesh){
	glm::vec3 mesh_min = toGLM(themesh->bbox.min);
	glm::vec3 mesh_max = toGLM(themesh->bbox.max);
	glm::vec3 lengths = mesh_max-mesh_min;
	float maxlength = glm::max(glm::max(lengths.x, lengths.y), lengths.z);
	for(int i=0; i<3;i++){
		float delta = maxlength - lengths[i];
		if(delta != 0){
			mesh_min[i] = mesh_min[i] - (delta / 2.0f);
			mesh_max[i] = mesh_max[i] + (delta / 2.0f);
		}
	}
	return AABox<glm::vec3>(mesh_min,mesh_max);
}

inline glm::vec3 computeFaceNormal(trimesh::TriMesh *themesh, size_t facenumber){
	trimesh::TriMesh::Face face = themesh->faces[facenumber];
	trimesh::vec3 &v0 = themesh->vertices[face[0]];
	trimesh::vec3 &v1 = themesh->vertices[face[1]];
	trimesh::vec3 &v2 = themesh->vertices[face[2]];
	return glm::normalize(glm::cross(toGLM(v0-v1),toGLM(v1-v2)));
}

inline glm::vec3 getShadingFaceNormal(trimesh::TriMesh *themesh, size_t facenumber){
	trimesh::TriMesh::Face face = themesh->faces[facenumber];
	trimesh::vec3 &n0 = themesh->normals[face[0]];
	trimesh::vec3 &n1 = themesh->normals[face[1]];
	trimesh::vec3 &n2 = themesh->normals[face[2]];
	glm::vec3 to_normalize;
	to_normalize.x = (n0[0] + n1[0] + n2[0]) / 3.0f; 
	to_normalize.y = (n0[1] + n1[1] + n2[1]) / 3.0f; 
	to_normalize.z = (n0[2] + n1[2] + n2[2]) / 3.0f;
	return glm::normalize(to_normalize);
}