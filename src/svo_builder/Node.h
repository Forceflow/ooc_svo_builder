#pragma once

#include "VoxelData.h"

#define NOCHILD -1
#define NODATA 0

// This is how an array of a leaf node will look
const char LEAF[8] = {NOCHILD, NOCHILD, NOCHILD, NOCHILD, NOCHILD, NOCHILD, NOCHILD, NOCHILD};

// An SVO node. Only contains child pointers, extend this if you want parent pointers as well
class Node
{
public:
	size_t data;
	size_t children_base;
	char children_offset[8];

	VoxelData data_cache; // only if you want to refine octree (clustering)

	Node();
	bool hasChild(unsigned int i) const;
	size_t getChildPos(unsigned int i) const;
	bool isLeaf() const;
	bool hasData() const;
	bool isNull() const;
};

// Default constructor
inline Node::Node() : data(0), children_base(0), data_cache(VoxelData()){
	memset(children_offset, static_cast<char>(NOCHILD), 8);
}

// Check if this Node has a child at position i
inline bool Node::hasChild(unsigned int i) const{
	return !(children_offset[i] == NOCHILD);
}

// Get the full index of the child at position i
inline size_t Node::getChildPos(unsigned int i) const{
	if(children_offset[i] == NOCHILD){
		return 0;
	} else {
		return children_base + children_offset[i];
	}
}

// If this node doesn't have data and is a leaf node, it's a null node
inline bool Node::isNull() const{
	return isLeaf() && !hasData();
}

// If this node doesn;t have any children, it's a leaf node
inline bool Node::isLeaf() const{
	if (memcmp(children_offset, LEAF, 8 * sizeof(char)) == 0){
		return true;
	}
	return false;
}

// If the data pointer is NODATA, there is no data
inline bool Node::hasData() const{
	return !(data == NODATA);
}