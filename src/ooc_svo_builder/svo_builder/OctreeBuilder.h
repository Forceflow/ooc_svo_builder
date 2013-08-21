#ifndef OCTREE_BUILDER_H_
#define OCTREE_BUILDER_H_

#include "globals.h"
#include <tri_util.h>
#include "util.h"
#include "Node.h"
#include "morton.h"
#include <stdio.h>
#include <fstream>
#include <assert.h>

using namespace std;
using namespace trimesh;

// Internal format to represent an octree
struct OctreeInfo {
	int version;
	size_t gridlength;
	size_t n_nodes;
	size_t n_data;

	OctreeInfo(){} 
};

size_t writeDataPoint(FILE* data_out, const DataPoint &d, size_t &b_data_pos);
size_t writeNode(FILE* node_out, const Node &n, size_t &b_node_pos);
void writeOctreeHeader(const std::string &filename, const OctreeInfo &i);

// Octreebuilder class. You pass this class DataPoints, it builds an octree from them.
class OctreeBuilder {
public:
	vector< vector< Node > > b_buffers;
	size_t gridlength;
	int b_maxdepth; // maximum octree depth
	uint64_t b_current_morton; // current morton position
	uint64_t b_max_morton; // maximum morton position
	size_t b_data_pos; // current output data position (array index)
	size_t b_node_pos; // current output node position (array index)

	// configuration
	bool fast_empty; // switch to enable fast_empty optimization
	bool generate_levels; // switch to enable basic generation of higher octree levels

	FILE* node_out;
	FILE* data_out;
	string octreeheader_name;

	OctreeBuilder(std::string basefilename, size_t gridlength, bool fast_empty, bool generate_levels);
	void addDataPoint(uint64_t morton_number, DataPoint point);
	void finalizeTree();

private:
	bool isBufferEmpty(const vector<Node> &buffer);
	Node groupNodes(const vector<Node> &buffer);
	void addEmptyDataPoint(int buffer);
	int highestNonEmptyBuffer();
	int computeBestFillBuffer(size_t budget);
	void fastAddEmpty(size_t budget);
};

// Check if a buffer contains non-empty nodes
inline bool OctreeBuilder::isBufferEmpty(const vector<Node> &buffer){
	for(int k = 0; k<8; k++){
		if(!buffer[k].isNull()){
			return false;
		}
	}
	return true;
}

// Find the highest non empty buffer, return its index
inline int OctreeBuilder::highestNonEmptyBuffer(){
	int highest_found = b_maxdepth; // highest means "lower in buffer id" here.
	for(int k = b_maxdepth; k>=0; k--){
		if(b_buffers[k].size() == 0){ // this buffer level is empty
			highest_found--;
		} else { // this buffer level is nonempty: break
			return highest_found;
		}
	}
	return highest_found;
}

// Compute the best fill buffer given the budget
inline int OctreeBuilder::computeBestFillBuffer(size_t budget){
	// which power of 8 fits in budget?
	int budget_buffer_suggestion = b_maxdepth-findPowerOf8(budget);
	// if our current guess is already b_maxdepth, return that, no need to test further
	if(budget_buffer_suggestion == b_maxdepth){return b_maxdepth;}
	// check highest empty buffer : there's a possibility to up the ante
	int highest_nonempty_buffer = highestNonEmptyBuffer();
	// best fill buffer is minimum of these
	return max(budget_buffer_suggestion, highest_nonempty_buffer);
}

// A method to quickly add empty nodes
inline void OctreeBuilder::fastAddEmpty(size_t budget){
	int buffer;
	while (budget > 0){
		buffer = computeBestFillBuffer(budget);
		addEmptyDataPoint(buffer);
		size_t budget_hit = pow(8.0,b_maxdepth-buffer);
		budget = budget - budget_hit;
	}

}

// Write a data point to file
inline size_t writeDataPoint(FILE* data_out, const DataPoint &d, size_t &b_data_pos){

	algo_timer.stop(); io_timer_out.start(); // TIMING

	fwrite(& d.opacity, sizeof(float), 1, data_out);
	fwrite(& d.color[0], sizeof(float), 3, data_out);
	fwrite(& d.normal[0], sizeof(float), 3, data_out);

	io_timer_out.stop(); algo_timer.start(); // TIMING

	b_data_pos++;
	return b_data_pos-1;
}

// Write an octree node to file
inline size_t writeNode(FILE* node_out, const Node &n, size_t &b_node_pos){

	algo_timer.stop(); io_timer_out.start(); // TIMING

	fwrite(& n.children_base, sizeof(size_t), 1, node_out);
	fwrite(& n.children_offset[0], sizeof(char), 8, node_out);
	fwrite(& n.data, sizeof(size_t), 1, node_out);

	io_timer_out.stop(); algo_timer.start(); // TIMING

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

#endif  // OCTREE_BUILDER_H_