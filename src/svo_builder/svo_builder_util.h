#ifndef UTIL_H_
#define UTIL_H_

#include <time.h>
#include <vector>
#include <sstream>
#include <TriMesh.h>
#include "timer.h"
#include "../libs/libmorton/include/morton3D_64.h"

using namespace trimesh;
using namespace std;

// Various math/helper stuff

// helper method to convert morton number to RGB color, for debug coloring purposes
inline vec3 mortonToRGB(const uint64_t morton_number, const size_t gridsize){
	unsigned int x,y,z;
	morton3D_64_Decode_LUT_shifted(morton_number,z,y,x);
	return vec3((float)x/gridsize, (float)y/gridsize, (float)z/gridsize);
}

template <size_t D, class T>
inline Vec<D,T> average3Vec(const Vec<D,T> v0, const Vec<D,T> v1, const Vec<D,T> v2){
	Vec<D,T> answer;
	for (size_t i = 0; i < D; i++){
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

#endif /* UTIL_H_ */
