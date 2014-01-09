#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdio.h>
#include <vector>
#include <tri_util.h>
#include <tri_tools.h>
#include "globals.h"
#include "intersection.h"

using namespace std;
using namespace trimesh;

// A Buffer which checks triangles against a bounding box, and writes them in batches to a given file/stream if they fit.
class Buffer{
public:
	FILE* file; // the file we'll write our triangles to
	string filename; // filename of the file we're writing to
	AABox<vec3> bbox_world; // bounding box of the morton grid this buffer represents, in world coords
	size_t n_triangles; // number of triangles already in

	// Buffered
	vector<Triangle> triangle_buffer; // triangle buffer
	size_t buffer_max; // maximum of tris we buffer before writing to disk

	Buffer();
	Buffer(const std::string &filename, AABox<vec3> bbox_world, size_t buffer_max);
	~Buffer();

	void processTriangle(Triangle &t, const AABox<vec3> &bbox);

private:
	void flush();
};

// default constructor
inline Buffer::Buffer() : bbox_world(AABox<vec3>(vec3(),vec3(1,1,1))), n_triangles(0), buffer_max(1024), file(NULL), filename(""){
}

// full constructor
inline Buffer::Buffer(const std::string &filename, AABox<vec3> bbox_world, size_t buffer_max): bbox_world(bbox_world), n_triangles(0), buffer_max(buffer_max), file(NULL), filename(filename) {
	triangle_buffer.reserve(buffer_max); // prepare buffer
	file = NULL;
}

//destructor
inline Buffer::~Buffer(){
	if(buffer_max != 0){
		flush();
	}
	if(file != NULL){ // only close the file if we opened it.
		fclose(file);
	}
}

// Flush the buffer and write everything to disk
inline void Buffer::flush(){
	if(triangle_buffer.size() == 0){
		return; // nothing to flush here.
	}
	if(file == NULL){ // if the file is not open yet, we open it.
		file = fopen(filename.c_str(), "wb");
	}
	part_algo_timer.stop(); part_io_out_timer.start(); // TIMING
	writeTriangles(file,triangle_buffer[0],triangle_buffer.size());
	part_io_out_timer.stop(); part_algo_timer.start();  // TIMING
	triangle_buffer.clear();
}

// Check triangle against buffer bounding box and add it to buffer if it is in it.
inline void Buffer::processTriangle(Triangle &t, const AABox<vec3> &bbox){
	if(intersectBoxBox(bbox, bbox_world)){ // triangle in this partition
		if(buffer_max == 0){ // no buffering, just write triangle
			part_algo_timer.stop(); part_io_out_timer.start(); // TIMING
			writeTriangle(file, t);
			part_io_out_timer.stop(); part_algo_timer.start();  // TIMING
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