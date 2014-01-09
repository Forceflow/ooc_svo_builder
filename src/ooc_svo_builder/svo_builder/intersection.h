#ifndef INTERSECTION_H_
#define INTERSECTION_H_

#include <TriMesh.h>
#include <cassert>
#include <tri_util.h>
#include "geometry_primitives.h"

// Intersection methods
inline AABox<vec3> computeBoundingBox(const vec3 &v0, const vec3 &v1, const vec3 &v2){
	AABox<vec3> answer; 
	answer.min[0] = min(v0[0],min(v1[0],v2[0]));
	answer.min[1] = min(v0[1],min(v1[1],v2[1]));
	answer.min[2] = min(v0[2],min(v1[2],v2[2]));
	answer.max[0] = max(v0[0],max(v1[0],v2[0]));
	answer.max[1] = max(v0[1],max(v1[1],v2[1]));
	answer.max[2] = max(v0[2],max(v1[2],v2[2]));
	return answer;
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
	if(len2(dist) > cyl.radius_squared){
		return false;
	} 
	return true;
}

template <typename T>
inline bool intersectBoxBox(const AABox<T> &a, const AABox<T> &b){
	if (a.max[0] < b.min[0] || a.max[1] < b.min[1] || a.max[2] < b.min[2] || a.min[0] > b.max[0] || a.min[1] > b.max[1] || a.min[2] > b.max[2]) { return false; }
	return true; // intersection or inside
}

#endif /*INTERSECTION_H_*/
