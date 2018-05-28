#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "timer.h"
#include "../libs/libmorton/include/morton.h"

using namespace glm;

// Various math/helper stuff

// helper method to convert morton number to RGB color, for debug coloring purposes
inline vec3 mortonToRGB(const uint_fast64_t morton_number, const size_t gridsize){
	uint_fast32_t x,y,z;
	morton3D_64_decode(morton_number,z,y,x);
	return vec3((float)x/gridsize, (float)y/gridsize, (float)z/gridsize);
}

inline vec3 average3Vec(const vec3 v0, const vec3 v1, const vec3 v2){
	vec3 answer;
	for (unsigned int i = 0; i < 3; i++){
		answer[i] = (v0[i] + v1[i] + v2[i]) / 3.0f;
	}
	return answer;
}

inline unsigned int findPowerOf8(size_t n){
	if(n == 0){return 0;}
	unsigned int highest_index = 0;
	while(n >>= 1){
		highest_index++;
	}
	return (highest_index/3);
}

inline unsigned int log2(unsigned int val) {
	unsigned int ret = -1;
	while (val != 0) {
		val >>= 1;
		ret++;
	}
	return ret;
}

inline int isPowerOf2(unsigned int x){
  return ((x != 0) && !(x & (x - 1)));
}

template <typename T> T clampval(const T& value, const T& low, const T& high) {
  return value < low ? low : (value > high ? high : value); 
}
