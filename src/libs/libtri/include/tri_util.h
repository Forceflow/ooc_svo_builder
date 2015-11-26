#ifndef TRI_UTIL_H_
#define TRI_UTIL_H_
#include <TriMesh.h>
#include <sstream>

using namespace std;
using namespace trimesh;

#ifdef BINARY_VOXELIZATION
#define TRIANGLE_SIZE 9 // just the vertices
#else
#define TRIANGLE_SIZE 21 // vertices + normal + vertex colors
#endif

// Various helper methods / structs

// Custom value to string method to avoid C++11 dependency causing fopenmp problems in OSX
template <typename T>
string val_to_string( T Number ) {
	stringstream ss;
	ss << Number;
	return ss.str();
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
	Color v0_color;
	Color v1_color;
	Color v2_color;

	// Default constructor
	Triangle(): v0(vec3()), v1(vec3()), v2(vec3()), normal(vec3()), v0_color(Color()),v1_color(Color()),v2_color(Color()){}

	// Constructor with all fields
	Triangle(vec3 v0, vec3 v1, vec3 v2, vec3 normal, Color v0_color, Color v1_color, Color v2_color): v0(v0), v1(v1), v2(v2), normal(normal),
		v0_color(v0_color), v1_color(v1_color),v2_color(v2_color){}
};
#endif

#endif
