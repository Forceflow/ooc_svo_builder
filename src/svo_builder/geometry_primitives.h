#pragma once
#include <glm/glm.hpp>

using namespace glm;

// Several geometry primitives

struct Plane {
	vec3 normal;
	float D;

	Plane(vec3 normal,  float D): normal(normal), D(D){}
	Plane(vec3 p0, vec3 p1, vec3 p2){
		normal = normalize(cross(p1 - p0, p2 - p1));
		D = -1.0f * dot(normal, p0);
	}
	Plane(vec3 normal, vec3 pointonplane){
		normal = normalize(normal);
		D = -1.0f * dot(normal, pointonplane);
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
		vec3 x = p2 - p1;
		length_squared = dot(x,x);
	}
};