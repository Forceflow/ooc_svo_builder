#pragma once

#include "tri_tools.h"
#include <stdio.h>

using namespace std;

// A class to read triangles from a .tridata file.
class TriReader{
	size_t n_triangles;
	size_t n_read; 
	size_t n_served; 

	size_t current_tri; // current triangle id we're going to read

	size_t buffersize; 
	Triangle* buffer;

	FILE* file;

public:
	TriReader();
	TriReader(const TriReader&);
	TriReader(const std::string &filename, size_t n_triangles, size_t buffersize);
	void getTriangle(Triangle& t);
	Triangle getTriangle();
	bool hasNext();
	~TriReader();
private:
	void fillBuffer();
};

inline TriReader::TriReader(){
	// TODO
}

inline TriReader::TriReader(const TriReader&){
	// TODO
}

inline TriReader::TriReader(const std::string &filename, size_t n_triangles, size_t buffersize): n_triangles(n_triangles), buffersize(buffersize), n_read(0), current_tri(0), n_served(0){
	// prepare buffer
	buffer = new Triangle[buffersize];
	// prepare file
	file = fopen(filename.c_str(), "rb");
	// fill Buffer
	fillBuffer();
}

inline Triangle TriReader::getTriangle(){
	if(current_tri == buffersize){ // at end of buffer, refill it
		fillBuffer();
		current_tri = 0;
	}
	Triangle t = buffer[current_tri]; // assign triangle from buffer
	current_tri++; // set index for next triangle
	n_served++;
	return t;
}

inline void TriReader::getTriangle(Triangle& t){
	if(current_tri == buffersize){ // at end of buffer, refill it
		fillBuffer();
		current_tri = 0;
	}
	t = buffer[current_tri]; // assign triangle from buffer
	current_tri++; // set index for next triangle
	n_served++;
}

inline bool TriReader::hasNext(){
	return (n_served < n_triangles);
}

inline void TriReader::fillBuffer(){
	size_t readcount = glm::min(buffersize, n_triangles - n_read); // don't read more than there are
	readTriangles(file,buffer[0],readcount); // read new triangles
	n_read += readcount; // update the number of tri's we've read
}

inline TriReader::~TriReader(){
	delete buffer;
	fclose(file);
}