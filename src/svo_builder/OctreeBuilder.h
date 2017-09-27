#pragma once

#include <cstdint>
#include <stdio.h>
#include <algorithm>
#include <glm/glm.hpp>
#include "../libs/libtri/include/tri_util.h"
#include "globals.h"
#include "svo_builder_util.h"
#include "octree_io.h"

using namespace std;
using namespace glm;

// Octreebuilder class. You pass this class DataPoints, it builds an octree from them.
class OctreeBuilder {
public:
	vector< vector< Node > > b_buffers;
	size_t gridlength;
	int b_maxdepth; // maximum octree depth
	uint_fast64_t b_current_morton; // current morton position
	uint_fast64_t b_max_morton; // maximum morton position
	size_t b_data_pos; // current output data position (array index)
	size_t b_node_pos; // current output node position (array index)

	// configuration
	bool generate_levels; // switch to enable basic generation of higher octree levels

	FILE* node_out;
	FILE* data_out;
	string base_filename;

	OctreeBuilder(std::string base_filename, size_t gridlength, bool generate_levels);
	void finalizeTree();
	void addVoxel(const uint_fast64_t morton_number);
	void addVoxel(const VoxelData& point);

private:
	// helper methods for octree building
	void fastAddEmpty(const size_t budget);
	void addEmptyVoxel(const int buffer);
	bool isBufferEmpty(const vector<Node> &buffer);
	void refineBuffers(const int start_depth);
	Node groupNodes(const vector<Node> &buffer);
	int highestNonEmptyBuffer();
	int computeBestFillBuffer(const size_t budget);
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
inline int OctreeBuilder::computeBestFillBuffer(const size_t budget){
	// which power of 8 fits in budget?
	int budget_buffer_suggestion = b_maxdepth-findPowerOf8(budget);
	// if our current guess is already b_maxdepth, return that, no need to test further
	if(budget_buffer_suggestion == b_maxdepth){return b_maxdepth;}
	// best fill buffer is maximum of suggestion and highest non_empty buffer
	return std::max(budget_buffer_suggestion, highestNonEmptyBuffer());
}

// A method to quickly add empty nodes
inline void OctreeBuilder::fastAddEmpty(const size_t budget){
	size_t r_budget = budget;
	while (r_budget > 0){
		unsigned int buffer = computeBestFillBuffer(r_budget);
		addEmptyVoxel(buffer);
		size_t budget_hit = static_cast<size_t>(pow(8.0,b_maxdepth-buffer));
		r_budget = r_budget - budget_hit;
	}
}