#ifndef DATAPOINT_H_
#define DATAPOINT_H_

#include <TriMesh.h>

using namespace trimesh;

// Class representing a voxel payload
class DataPoint {
public:
	float opacity;
	vec3 color;
	vec3 normal;

	DataPoint();
	DataPoint(float opacity, vec3 color);
	DataPoint(float opacity, vec3 color, vec3 normal);
	bool isEmpty();
};


inline DataPoint::DataPoint() : opacity(0.0f), color(vec3(0,0,0)), normal(vec3(0.0f,0.0f,0.0f)){
}

inline DataPoint::DataPoint(float opacity, vec3 color) : opacity(opacity), color(color){
}

inline DataPoint::DataPoint(float opacity, vec3 color, vec3 normal) : opacity(opacity), color(color), normal(normal){
}

inline bool DataPoint::isEmpty(){
	return (opacity == 0.0f);
}

#endif // DATAPOINT_H_