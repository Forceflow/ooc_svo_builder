#ifndef BUFFER_H_
#define BUFFER_H_

#include <tri_util.h>
#include <tri_tools.h>
#include "intersection.h"
#include "globals.h"
#include <stdio.h>
#include <vector>

using namespace std;
using namespace trimesh;

// A Buffer which checks triangles against a bounding box, and writes them in batches to a given file/stream if they fit.
class Buffer{
public:
	//ofstream file;
	FILE* file;
	AABox<vec3> bbox_world;
	size_t n_triangles;
	
	// Buffered
	vector<Triangle> triangle_buffer;
	size_t buffer_max;

	Buffer();
	Buffer(const std::string &filename, AABox<vec3> bbox_world, size_t buffer_max);
	~Buffer();

	void processTriangle(Triangle &t, const AABox<vec3> &bbox);

private:
	void flush();
};

// Flush the buffer and write everything to disk
inline void Buffer::flush(){
	algo_timer.stop(); io_timer_out.start(); // TIMING
	writeTriangles(file,triangle_buffer[0],triangle_buffer.size());
	io_timer_out.stop(); algo_timer.start();  // TIMING
	triangle_buffer.clear();
}

// Check triangle against buffer bounding box and add it to buffer if it is in it.
inline void Buffer::processTriangle(Triangle &t, const AABox<vec3> &bbox){
	if(intersectBoxBox(bbox, bbox_world)){ // triangle in this partition
		if(buffer_max == 0){ // no buffering, just write triangle
			algo_timer.stop(); io_timer_out.start(); // TIMING
			writeTriangle(file, t);
			io_timer_out.stop(); algo_timer.start();  // TIMING
		} else { // add to buffer
			triangle_buffer.push_back(t);
			if(triangle_buffer.size() >= buffer_max) { // buffer full, writeout to files
				flush();
			}
		}
		n_triangles++;
	}
}


#endif // BUFFER_H_