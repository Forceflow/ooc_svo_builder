#pragma once 

#include <glm/glm.hpp>
#include <sstream>
#include <string>

#ifdef BINARY_VOXELIZATION
#define TRIANGLE_SIZE 9 // just the vertices
#else
#define TRIANGLE_SIZE 21 // vertices + normal + vertex colors
#endif

// Custom value to string method to avoid C++11 dependency causing fopenmp problems in OSX
template <typename T>
std::string val_to_string( T Number ) {
	std::stringstream ss;
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
	glm::vec3 v0;
	glm::vec3 v1;
	glm::vec3 v2;

	Triangle(): v0(glm::vec3()), v1(glm::vec3()), v2(glm::vec3()) {}
	Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2): v0(v0), v1(v1), v2(v2) {}
};
#else
struct Triangle {
	glm::vec3 v0;
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 normal;
	glm::vec3 v0_color;
	glm::vec3 v1_color;
	glm::vec3 v2_color;

	// Default constructor
	Triangle(): v0(glm::vec3()), v1(glm::vec3()), v2(glm::vec3()), normal(glm::vec3()), v0_color(glm::vec3()),v1_color(glm::vec3()),v2_color(glm::vec3()){}

	// Constructor with all fields
	Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 normal, glm::vec3 v0_color, glm::vec3 v1_color, glm::vec3 v2_color): v0(v0), v1(v1), v2(v2), normal(normal),
		v0_color(v0_color), v1_color(v1_color),v2_color(v2_color){}
};
#endif
