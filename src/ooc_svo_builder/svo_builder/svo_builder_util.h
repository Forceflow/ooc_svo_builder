#ifndef UTIL_H_
#define UTIL_H_

#include <time.h>
#include <vector>
#include <sstream>
#include <TriMesh.h>
#include "morton.h"

using namespace trimesh;
using namespace std;

// Various math/helper stuff

// helper method to convert morton number to RGB color, for debug coloring purposes
inline vec3 mortonToRGB(const uint64_t morton_number, const size_t gridsize){
	int x,y,z;
	mortonDecode(morton_number,z,y,x);
	return vec3((float)x/gridsize, (float)y/gridsize, (float)z/gridsize);
}

// TODO: make this more general
inline vec3 averageVec3(const vec3 v0, const vec3 v1, const vec3 v2){
	vec3 answer;
	answer[0] = (v0[0] + v1[0] + v2[0]) / 3.0f;
	answer[1] = (v0[1] + v1[1] + v2[2]) / 3.0f;
	answer[2] = (v0[2] + v1[2] + v2[3]) / 3.0f;
	return answer;
}

inline int findPowerOf8(size_t n){
	if(n == 0){return 0;}
	int highest_index = 0;
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

// Simple timer to measure time, using CLOCKS_PER_SEC
// Should work on all platforms, sacrificing some precision.
struct Timer {
	clock_t Start;
	clock_t Elapsed;
	Timer(){
		Elapsed = 0;
		Start = clock();
	}
	void reset(){
		Start = clock();
	}
	void resetTotal(){
		Elapsed = 0;
	}
	void start(){
		Start = clock();
	}
	void stop(){
		clock_t End = clock();
		Elapsed = Elapsed + (End - Start);
	}
	double getTotalTimeSeconds() const{
		return ((double) Elapsed)/ ((double) CLOCKS_PER_SEC);
	}
};

template <typename T> T clampval(const T& value, const T& low, const T& high) {
  return value < low ? low : (value > high ? high : value); 
}

#endif /* UTIL_H_ */
