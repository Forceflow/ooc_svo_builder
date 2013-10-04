#ifndef MORTON_H_
#define MORTON_H_

#include <stdint.h>
#include <limits.h>

using namespace std;

uint64_t mortonEncode(int x, int y, int z);
void mortonDecode(uint64_t morton, int& x, int& y, int& z);

// encode a given (x,y,z) coordinate to a 64-bit morton code
inline uint64_t mortonEncode(int x, int y, int z){
	uint64_t answer = 0;
	for (uint64_t i = 0; i < (sizeof(int) * CHAR_BIT); ++i) {
		answer |= ((x & ((uint64_t)1 << i)) << 2*i) | ((y & ((uint64_t)1 << i)) << (2*i + 1)) | ((z & ((uint64_t)1 << i)) << (2*i + 2));
	}
	return answer;
}

// decode a given 64-bit morton code to an integer (x,y,z) coordinate
inline void mortonDecode(uint64_t morton, int& x, int& y, int& z){
	x = 0;
	y = 0;
	z = 0;
	for (uint64_t i = 0; i < (sizeof(uint64_t) * CHAR_BIT)/3; ++i) {
		x |= ((morton & (uint64_t( 1ull ) << uint64_t((3ull * i) + 0ull))) >> uint64_t(((3ull * i) + 0ull)-i));
		y |= ((morton & (uint64_t( 1ull ) << uint64_t((3ull * i) + 1ull))) >> uint64_t(((3ull * i) + 1ull)-i));
		z |= ((morton & (uint64_t( 1ull ) << uint64_t((3ull * i) + 2ull))) >> uint64_t(((3ull * i) + 2ull)-i));
	}
}

#endif // MORTON_H_
