#ifndef TRI_UTIL_H_
#define TRI_UTIL_H_
#include <TriMesh.h>

using namespace std;
using namespace trimesh;

inline bool file_exists(const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}

template <typename T>
struct AABox {
	T min;
	T max;
	AABox(): min(T()), max(T()){}
	AABox(T min, T max): min(min), max(max){}
};

#ifdef BINARY_VOXELIZATION
struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;

	Triangle(): v0(vec3()), v1(vec3()), v2(vec3()) {}
	Triangle(vec3 v0, vec3 v1, vec3 v2): v0(v0), v1(v1), v2(v2) {}
};
#else
struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 normal;

	Triangle(): v0(vec3()), v1(vec3()), v2(vec3()), normal(vec3()){}
	Triangle(vec3 v0, vec3 v1, vec3 v2, vec3 normal): v0(v0), v1(v1), v2(v2), normal(normal){}
};
#endif

#endif
