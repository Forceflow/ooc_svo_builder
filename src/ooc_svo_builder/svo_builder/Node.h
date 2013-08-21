#ifndef NODE_H_
#define NODE_H_

#pragma once

#include "DataPoint.h"
#include <string>
#include <cstdint>

#define NOCHILD -1

// An SVO node. Only contains child pointers, extend this if you want parent pointers as well
class Node
{
public:
	size_t children_base;
	char children_offset[8];
	size_t data;

	DataPoint data_cache; // only if you want to refine octree (clustering)

	Node();
	bool hasChild(int i) const;
	size_t getChildPos(int i) const;
	bool isLeaf() const;
	bool hasData() const;
	bool isNull() const;
};

inline Node::Node() : data(0), children_base(0), data_cache(DataPoint()) {
	for(int i = 0; i<8; i++){
		children_offset[i] = NOCHILD;
	}
}

// Check if this Node has a child at position i
inline bool Node::hasChild(int i) const{
	if(children_offset[i] == NOCHILD){
		return false;
	}
	return true;
}

// Get the full index of the child at position i
inline size_t Node::getChildPos(int i) const{
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
	for(int i = 0; i<8; i++){if(children_offset[i] != NOCHILD){return false;}}
	return true;
}

inline bool Node::hasData() const{
	return !(data == 0);
}

#endif /* NODE_H_ */
