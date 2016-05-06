#ifndef TRI_TOOLS_H_
#define TRI_TOOLS_H_

#include <string>
#include <fstream>
#include <iostream>
#include "tri_util.h"
#include "file_tools.h"

using namespace std;

// A struct to contain all info read from .tri file headers
struct TriInfo{
	string base_filename;
	int version;
	int geometry_only;
	size_t n_triangles;
	AABox<glm::vec3> mesh_bbox;

	TriInfo() : base_filename(""), version(version), geometry_only(geometry_only), n_triangles(0), mesh_bbox(AABox<glm::vec3>()) {} // default constructor

	// print out Tri information
	void print() const{
		cout << "  base_filename: " << base_filename << endl;
		cout << "  tri version: " << version << endl;
		cout << "  geometry only: " << geometry_only << endl;
		cout << "  n_triangles: " << n_triangles << endl;
		cout << "  bbox min: " << mesh_bbox.min[0] << " " << mesh_bbox.min[1] << " " << mesh_bbox.min[2] << endl;
		cout << "  bbox max: " << mesh_bbox.max[0] << " " << mesh_bbox.max[1] << " " << mesh_bbox.max[2] << endl;
	}

	// check if all files required by Tri exist
	bool filesExist() const{
		string header = base_filename + string(".tri");
		string tridata = base_filename + string(".tridata");
		return (file_exists(header) && file_exists(tridata));
	}
};

// STDIO IO for Triangles
inline void readTriangle(FILE* f, Triangle &t){
	size_t read = fread(&t, TRIANGLE_SIZE*sizeof(float), 1, f);
}

inline void readTriangles(FILE* f, Triangle &t, size_t howmany){
	size_t read = fread(&t, TRIANGLE_SIZE*sizeof(float), howmany, f);
}

inline void writeTriangle(FILE* f, Triangle &t){
	fwrite(&t, TRIANGLE_SIZE*sizeof(float), 1, f);
}

inline void writeTriangles(FILE* f, Triangle &t, size_t howmany){
	fwrite(&t, TRIANGLE_SIZE*sizeof(float), howmany, f);
}

// FSTREAM IO for Triangles (deprecated - this slow)
inline void readTriangle(ifstream &file, Triangle &t){
	file.read(reinterpret_cast<char*> (&t.v0[0]), TRIANGLE_SIZE*sizeof(float));
}

inline void readTriangles(ifstream &file, Triangle &t, size_t howmany){
	file.read(reinterpret_cast<char*> (&t.v0[0]), howmany*TRIANGLE_SIZE*sizeof(float));
}

inline void writeTriangle(ofstream &file, Triangle &t){
	file.write(reinterpret_cast<char*> (&t.v0[0]), TRIANGLE_SIZE*sizeof(float));
}

inline void writeTriangles(ofstream &file, Triangle &t, size_t howmany){
	file.write(reinterpret_cast<char*> (&t.v0[0]), howmany*TRIANGLE_SIZE*sizeof(float));
}

// Parsing a .tri header file and store info in TriInfo struct
inline int parseTriHeader(std::string filename, TriInfo &t){
	ifstream file;
	file.open(filename.c_str(), ios::in);

	if(! file){
		cout << "  Error: file " << filename << " does not exist." << endl;
		return 0;
	}

	t.base_filename = filename.substr(0,filename.find_last_of("."));

	string line; file >> line;  // #tri
	if (line.compare("#tri") != 0) {
		cout << "  Error: first line reads [" << line << "] instead of [#tri]" << endl; return 0;
	}
	file >> t.version;

	bool done = false;
	t.geometry_only = 0;

	while(file.good() && !done) {
		file >> line;
		if (line.compare("END") == 0) {
			done = true; // when we encounter data keyword, we're at the end of the ASCII header
		} else if (line.compare("ntriangles") == 0) {
			file >> t.n_triangles;
		} else if (line.compare("geo_only") == 0) {
			file >> t.geometry_only;
		} else if (line.compare("bbox") == 0) {
			file >> t.mesh_bbox.min[0] >> t.mesh_bbox.min[1] >> t.mesh_bbox.min[2] >> t.mesh_bbox.max[0] >> t.mesh_bbox.max[1] >> t.mesh_bbox.max[2];
		} else { 
			cout << "  unrecognized keyword [" << line << "], skipping" << endl;
			char c; do { c = file.get(); } while(file.good() && (c != '\n'));
		}
	}
	if (!done) {
		cout << "  error reading header" << endl; return 0;
	}
	file.close();
	return 1;
}

// Write a .tri header file with info from TriInfo struct
inline void writeTriHeader(const std::string &filename, const TriInfo &t){
	ofstream outfile;
	outfile.open(filename.c_str(), ios::out);
	outfile << "#tri " << t.version << endl;
	outfile << "ntriangles " << t.n_triangles << endl;
	outfile << "geo_only " << t.geometry_only << endl;
	outfile << "bbox  " << t.mesh_bbox.min[0] << " " << t.mesh_bbox.min[1] << " " << t.mesh_bbox.min[2] << " " << t.mesh_bbox.max[0] << " " 
		<< t.mesh_bbox.max[1] << " " << t.mesh_bbox.max[2] << endl;
	outfile << "END" << endl;
	outfile.close();
}

#endif