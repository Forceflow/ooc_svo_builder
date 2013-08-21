#include "Buffer.h"

using namespace std;

// default constructor
Buffer::Buffer() : bbox_world(AABox<vec3>(vec3(0,0,0),vec3(1,1,1))), n_triangles(0), buffer_max(1024), file(NULL){
}

// full constructor
Buffer::Buffer(const std::string &filename, AABox<vec3> bbox_world, size_t buffer_max): bbox_world(bbox_world), n_triangles(0), buffer_max(buffer_max), file(NULL) {
#ifdef VERBOSE
	cout << "  opening file : " << filename << endl;
#endif
	// prepare buffer
	triangle_buffer.reserve(buffer_max);
	// prepare file
	file = fopen(filename.c_str(), "wb");
}

//destructor
Buffer::~Buffer(){
	if(buffer_max != 0){
		flush();
	}
	fclose(file);
}