#ifndef INTERSECTION_H_
#define INTERSECTION_H_

#include <TriMesh.h>
#include <cassert>
#include <tri_util.h>
#include "geometry_primitives.h"

// Intersection methods
inline AABox<vec3> computeBoundingBox(const vec3 &v0, const vec3 &v1, const vec3 &v2){
	return AABox<vec3>(min(v0,min(v1,v2)),max(v0,max(v1,v2)));
}

inline bool isPointBetweenParallelPlanes(const vec3 &point, const Plane &a, const Plane &b){
	// test if planes are parallel
	assert((a.normal CROSS b.normal) == vec3(0,0,0) && "These planes should be parallel.");
	return ((a.normal DOT point) + a.D) * ((b.normal DOT point) + b.D) < 0.0;
}

inline bool isPointInSphere(const vec3 &point, const Sphere &sphere){
	return sphere.radius_squared >= len2(sphere.center-point);
}

inline bool isPointinCylinder(const vec3 &point, const Cylinder &cyl){
	vec3 AB = cyl.p2 - cyl.p1;
	vec3 AX = point - cyl.p1;
	// project pdiff on diff, check length of resulting vector
	float dot = (AX DOT AB);
	if(dot < 0.0f){
		return false;
	}
	dot = dot /len2(AB);
	if(dot > 1.0f){
		return false;
	}
	vec3 dist = AX - dot*AB; 
	float dist_squared = len2(dist);
	if(len2(dist) > cyl.radius_squared){
		return false;
	} else {
		return true;
	}
}

template <typename T>
inline bool intersectBoxBox(const AABox<T> &a, const AABox<T> &b){
	if(a.max[0] < b.min[0]) {return false;}
	if(a.min[0] > b.max[0]) {return false;}
	if(a.max[1] < b.min[1]) {return false;}
	if(a.min[1] > b.max[1]) {return false;}
	if(a.max[2] < b.min[2]) {return false;}
	if(a.min[2] > b.max[2]) {return false;}
	return true; // intersection or inside
}


#endif /*INTERSECTION_H_*/
