#ifndef OCTREE_IO_H_
#define OCTREE_IO_H_

#include <stdio.h>
#include <fstream>
#include "Node.h"
#include "DataPoint.h"

using namespace std;

// File containing all the octree IO methods

// Internal format to represent an octree
struct OctreeInfo {
	int version;
	string base_filename;
	size_t gridlength;
	size_t n_nodes;
	size_t n_data;

	OctreeInfo() : version(1), base_filename(string("")), gridlength(1024), n_nodes(0), n_data(0) {}
	OctreeInfo(int version, string base_filename, size_t gridlength, size_t n_nodes, size_t n_data) : version(version), base_filename(base_filename), gridlength(gridlength), n_nodes(n_nodes), n_data(n_data) {} 

	void print(){
		cout << "  version: " << version << endl;
		cout << "  base_filename: " << base_filename << endl;
		cout << "  grid length: " << gridlength << endl;
		cout << "  n_nodes: " << n_nodes << endl;
		cout << "  n_data: " << n_data << endl;
	}
};

size_t writeDataPoint(FILE* data_out, const DataPoint &d, size_t &b_data_pos);
void readDataPoint(FILE* f, DataPoint &d);
size_t writeNode(FILE* node_out, const Node &n, size_t &b_node_pos);
inline void readNode(FILE* f, Node &n);

void writeOctreeHeader(const std::string &filename, const OctreeInfo &i);
int parseOctreeHeader(const std::string &filename, OctreeInfo &i);

// Write a data point to file
inline size_t writeDataPoint(FILE* data_out, const DataPoint &d, size_t &b_data_pos){
	fwrite(& d.opacity, sizeof(float), 1, data_out);
	fwrite(& d.color[0], sizeof(float), 3, data_out);
	fwrite(& d.normal[0], sizeof(float), 3, data_out);
	b_data_pos++;
	return b_data_pos-1;
}

// Read a data point from a file
inline void readDataPoint(FILE* f, DataPoint &d){
	fread(& d.opacity, sizeof(float), 1, f);
	fread(& d.color[0], sizeof(float), 3, f);
	fread(& d.normal[0], sizeof(float), 3, f);
}

// Write an octree node to file
inline size_t writeNode(FILE* node_out, const Node &n, size_t &b_node_pos){
	fwrite(& n.children_base, sizeof(size_t), 1, node_out);
	fwrite(& n.children_offset[0], sizeof(char), 8, node_out);
	fwrite(& n.data, sizeof(size_t), 1, node_out);
	b_node_pos++;
	return b_node_pos-1;
}

// Read a Node from a file
inline void readNode(FILE* f, Node &n){
	fread(& n.children_base, sizeof(size_t), 1, f);
	fread(& n.children_offset[0], sizeof(char), 8, f);
	fread(& n.data, sizeof(size_t), 1, f);
}

// Write an octree header to a file
inline void writeOctreeHeader(const std::string &filename, const OctreeInfo &i){
	ofstream outfile;
	outfile.open(filename.c_str(), ios::out);
	outfile << "#octreeheader 1" << endl;
	outfile << "gridlength " << i.gridlength << endl;
	outfile << "n_nodes " << i.n_nodes << endl;
	outfile << "n_data " << i.n_data << endl;
	outfile << "END" << endl;
	outfile.close();
}

// Parse a given octree header, store info in OctreeInfo struct
inline int parseOctreeHeader(const std::string &filename, OctreeInfo &i){
	cout << "  reading octree header from " << filename << " ... " << endl;
	ifstream headerfile;
	headerfile.open(filename.c_str(), ios::in);

	i.base_filename = filename.substr(0,filename.find_last_of("."));

	string line; bool done = false;
	headerfile >> line >> i.version;
	if (line.compare("#octreeheader") != 0) {cout << "    Error: first line reads [" << line << "] instead of [#octreeheader]" << endl; return 0;}

	while(headerfile.good() && !done) {
		headerfile >> line;
		if (line.compare("END") == 0) done = true; // when we encounter data keyword, we're at the end of the ASCII header
		else if (line.compare("gridlength") == 0) {headerfile >> i.gridlength;}
		else if (line.compare("n_nodes") == 0) {headerfile >> i.n_nodes;}
		else if (line.compare("n_data") == 0) {headerfile >> i.n_data;}
		else { cout << "  unrecognized keyword [" << line << "], skipping" << endl;
		char c; do { c = headerfile.get(); } while(headerfile.good() && (c != '\n'));
		}
	}

	headerfile.close();
	return 1;
}

#endif