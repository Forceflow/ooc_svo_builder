#ifndef UTIL_H_
#define UTIL_H_

#include <time.h>
#include <vector>
#include <sstream>
#include <TriMesh.h>

using namespace trimesh;
using namespace std;

// Various math/helper stuff

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
	double getTotalTimeSeconds(){
		return ((double) Elapsed)/ ((double) CLOCKS_PER_SEC);
	}
};

template <typename T> T clampval(const T& value, const T& low, const T& high) {
  return value < low ? low : (value > high ? high : value); 
}

#endif /* UTIL_H_ */
