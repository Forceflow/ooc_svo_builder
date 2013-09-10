#ifndef OCTREE_BUILDER_H_
#define OCTREE_BUILDER_H_

#include "globals.h"
#include <tri_util.h>
#include "svo_builder_util.h"
#include "morton.h"
#include <stdio.h>
#include <fstream>
#include <assert.h>
#include "octree_io.h"

using namespace std;
using namespace trimesh;

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
	string base_filename;

	OctreeBuilder(std::string base_filename, size_t gridlength, bool fast_empty, bool generate_levels);
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
	while (budget > 0){
		int buffer = computeBestFillBuffer(budget);
		addEmptyDataPoint(buffer);
		size_t budget_hit = (size_t) pow(8.0,b_maxdepth-buffer);
		budget = budget - budget_hit;
	}

}

#endif  // OCTREE_BUILDER_H_