#ifndef TRIP_TOOLS_H_
#define TRIP_TOOLS_H_

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "tri_tools.h"
#include "file_tools.h"

using namespace std;

struct TripInfo {
	string base_filename;
	int version;
	int geometry_only;
	size_t gridsize;
	AABox<glm::vec3> mesh_bbox;
	vector<size_t> part_tricounts;
	size_t n_triangles;
	size_t n_partitions;
	
	// default constructor
	TripInfo() : base_filename(""), version(1), geometry_only(0), gridsize(0), n_triangles(0), n_partitions(0), mesh_bbox(AABox<glm::vec3>()) {} 
	// construct from TriInfo
	TripInfo(const TriInfo &t) : base_filename(t.base_filename), version(t.version), geometry_only(t.geometry_only), gridsize(0), mesh_bbox(t.mesh_bbox), n_triangles(t.n_triangles), n_partitions(0) {} 

	void print() const{
		cout << "  base_filename: " << base_filename << endl;
		cout << "  trip version: " << version << endl;
		cout << "  geometry only: " << geometry_only << endl;
		cout << "  gridsize: " << gridsize << endl;
		cout << "  bbox min: " << mesh_bbox.min[0] << " " << mesh_bbox.min[1] << " " << mesh_bbox.min[2] << endl;
		cout << "  bbox max: " << mesh_bbox.max[0] << " " << mesh_bbox.max[1] << " " << mesh_bbox.max[2] << endl;
		cout << "  n_triangles: " << n_triangles << endl;
		cout << "  n_partitions: " << n_partitions << endl;
		for(size_t i = 0; i< n_partitions; i++){
			cout << "  partition " << i << " - tri_count: " << part_tricounts[i] << endl;
		}
	}

	bool filesExist() const{
		string header = base_filename + string(".trip");
		for(size_t i = 0; i< n_partitions; i++){
			if(part_tricounts[i] > 0){ // we only require the file to be there if it contains any triangles.
				string part_data_filename = base_filename + string("_") + val_to_string(i) + string(".tripdata");
				if(!file_exists(part_data_filename)){
					return false;
				}
			}
		}
		return (file_exists(header));
	}
};

inline int parseTripHeader(const std::string &filename, TripInfo &t){
	ifstream file;
	file.open(filename.c_str(), ios::in);

	t.base_filename = filename.substr(0,filename.find_last_of("."));

	string line; file >> line;  // #trip
	if (line.compare("#trip") != 0) {
		cout << "  Error: first line reads [" << line << "] instead of [#trip]" << endl; return 0;
	}
	file >> t.version;

	bool done = false;
	t.geometry_only = 0;

	while(file.good() && !done) {
		file >> line;
		if (line.compare("END") == 0) {
			done = true; // when we encounter data keyword, we're at the end of the ASCII header
		} else if (line.compare("gridsize") == 0) {
			file >> t.gridsize;
		}  else if (line.compare("n_triangles") == 0) {
			file >> t.n_triangles;
		} else if (line.compare("geo_only") == 0) {
			file >> t.geometry_only;
		} else if (line.compare("bbox") == 0) {
			file >> t.mesh_bbox.min[0] >> t.mesh_bbox.min[1] >> t.mesh_bbox.min[2] >> t.mesh_bbox.max[0] >> t.mesh_bbox.max[1] >> t.mesh_bbox.max[2];
		} else if (line.compare("n_partitions") == 0) {
			file >> t.n_partitions; // read number of partitions
			t.part_tricounts.resize(t.n_partitions);
			int index;
			size_t tricount;
			for(size_t i = 0; i < t.n_partitions; i++){
				file >> index >> tricount;
				t.part_tricounts[index] = tricount;
			}
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

inline void writeTripHeader(const std::string &filename, const TripInfo &t){
	// open file for writing
	ofstream outfile;
	outfile.open(filename.c_str(), ios::out);
	// write ASCII header
	outfile << "#trip " << t.version << endl;
	outfile << "gridsize " << t.gridsize << endl;
	outfile << "n_triangles " << t.n_triangles << endl;
	outfile << "bbox  " << t.mesh_bbox.min[0] << " " << t.mesh_bbox.min[1] << " " << t.mesh_bbox.min[2] << " " << t.mesh_bbox.max[0] << " " 
		<< t.mesh_bbox.max[1] << " " << t.mesh_bbox.max[2] << endl;
#ifdef BINARY_VOXELIZATION
	outfile << "geo_only " << 1 << endl;
#else
	outfile << "geo_only " << 0 << endl;
#endif
	outfile << "n_partitions " << t.n_partitions << endl;

	for(size_t i = 0; i < t.n_partitions; i++){
		outfile << i << " " << t.part_tricounts[i] << endl;
	}
	outfile << "END" << endl;
}

#endif
