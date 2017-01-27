#pragma once

#include <cassert>
#include <algorithm>
#include "geometry_primitives.h"
#include "../libs/libtri/include/tri_util.h"

// Intersection methods
inline AABox<vec3> computeBoundingBox(const vec3 &v0, const vec3 &v1, const vec3 &v2){
	AABox<vec3> answer; 
	answer.min[0] = std::min(v0[0],std::min(v1[0],v2[0]));
	answer.min[1] = std::min(v0[1],std::min(v1[1],v2[1]));
	answer.min[2] = std::min(v0[2],std::min(v1[2],v2[2]));
	answer.max[0] = std::max(v0[0],std::max(v1[0],v2[0]));
	answer.max[1] = std::max(v0[1],std::max(v1[1],v2[1]));
	answer.max[2] = std::max(v0[2],std::max(v1[2],v2[2]));
	return answer;
}

inline bool isPointBetweenParallelPlanes(const vec3 &point, const Plane &a, const Plane &b){
	// test if planes are parallel
	assert((cross(a.normal, b.normal)) == vec3(0,0,0) && "These planes should be parallel.");
	return ((dot(a.normal,point)) + a.D) * ((dot(b.normal,point)) + b.D) < 0.0f;
}

inline bool isPointInSphere(const vec3 &point, const Sphere &sphere){
	return sphere.radius_squared >= dot(sphere.center-point, sphere.center-point);
}

inline bool isPointinCylinder(const vec3 &point, const Cylinder &cyl){
	vec3 AB = cyl.p2 - cyl.p1;
	vec3 AX = point - cyl.p1;
	// project pdiff on diff, check length of resulting vector
	float dotp = dot(AX , AB);
	if(dotp < 0.0f){
		return false;
	}
	dotp = dotp / dot(AB,AB);
	if(dotp > 1.0f){
		return false;
	}
	vec3 dist = AX - dotp*AB; 
	if(dot(dist,dist) > cyl.radius_squared){
		return false;
	} 
	return true;
}

template <typename T>
inline bool intersectBoxBox(const AABox<T> &a, const AABox<T> &b){
	if (a.max[0] < b.min[0] || a.max[1] < b.min[1] || a.max[2] < b.min[2] || a.min[0] > b.max[0] || a.min[1] > b.max[1] || a.min[2] > b.max[2]) { return false; }
	return true; // intersection or inside
}
