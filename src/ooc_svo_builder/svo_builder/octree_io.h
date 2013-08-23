#ifndef OCTREE_IO_H_
#define OCTREE_IO_H_

#include <stdio.h>
#include <fstream>
#include "Node.h"
#include "DataPoint.h"

// File containing all the octree IO methods

// Internal format to represent an octree
struct OctreeInfo {
	int version;
	size_t gridlength;
	size_t n_nodes;
	size_t n_data;

	OctreeInfo() : version(1), gridlength(1024), n_nodes(0), n_data(0) {}
	OctreeInfo(int version, size_t gridlength, size_t n_nodes, size_t n_data) : version(version), gridlength(gridlength), n_nodes(n_nodes), n_data(n_data) {} 
};

size_t writeDataPoint(FILE* data_out, const DataPoint &d, size_t &b_data_pos);
size_t writeNode(FILE* node_out, const Node &n, size_t &b_node_pos);
void writeOctreeHeader(const std::string &filename, const OctreeInfo &i);

// Write a data point to file
inline size_t writeDataPoint(FILE* data_out, const DataPoint &d, size_t &b_data_pos){
	fwrite(& d.opacity, sizeof(float), 1, data_out);
	fwrite(& d.color[0], sizeof(float), 3, data_out);
	fwrite(& d.normal[0], sizeof(float), 3, data_out);
	b_data_pos++;
	return b_data_pos-1;
}

// Write an octree node to file
inline size_t writeNode(FILE* node_out, const Node &n, size_t &b_node_pos){
	fwrite(& n.children_base, sizeof(size_t), 1, node_out);
	fwrite(& n.children_offset[0], sizeof(char), 8, node_out);
	fwrite(& n.data, sizeof(size_t), 1, node_out);
	b_node_pos++;
	return b_node_pos-1;
}

inline void writeOctreeHeader(const std::string &filename, const OctreeInfo &i){
	ofstream outfile;
	outfile.open(filename.c_str(), ios::out);
	outfile << "#octreeheader 1" << endl;
	outfile << "gridlength " << i.gridlength << endl;
	outfile << "n_nodes " << i.n_nodes << endl;
	outfile << "n_data " << i.n_data << endl;
	outfile << "end" << endl;
	outfile.close();
}

#endif