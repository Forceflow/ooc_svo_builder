#ifndef GEOMETRY_PRIMITIVES_H_
#define GEOMETRY_PRIMITIVES_H_
#include <TriMesh.h>

using namespace std;
using namespace trimesh;

// Several geometry primitives

struct Plane {
	vec3 normal;
	float D;

	Plane(vec3 normal,  float D): normal(normal), D(D){}
	Plane(vec3 p0, vec3 p1, vec3 p2){
		vec3 v1 = p1-p0;
		vec3 v2 = p2-p1;
		// LINUX fix -- can't handle vec3 normal = normalize((vec3) v1 CROSS v2) FOR SOME MAD REASON
		vec3 to_normalize = v1 CROSS v2; 
		normal = normalize(to_normalize);
		D = - (normal DOT p0);
	}
	Plane(vec3 normal, vec3 pointonplane){
		normal = normalize(normal);
		D = - (normal DOT pointonplane);
	}
};

struct Sphere {
	vec3 center;
	float radius_squared;

	Sphere(vec3 center, float radius): center(center){
		radius_squared = radius*radius;
	}
};

struct Cylinder {
	vec3 p1;
	vec3 p2;
	float length_squared;
	float radius_squared;

	Cylinder(vec3 p1, vec3 p2, float radius): p1(p1), p2(p2){
		radius_squared = radius*radius;
		length_squared = len2( (vec3) p2-p1);
	}
};

#endif // GEOMETRY_PRIMITIVES_H_