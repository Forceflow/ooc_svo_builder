#pragma once
#include <tri_util.h>

glm::vec3 InterpolateValue(const glm::vec3& barycentric, glm::vec3 vert1Val, glm::vec3 vert2Val, glm::vec3 vert3Val)
{
	return barycentric.x * vert1Val + barycentric.y * vert2Val + barycentric.z * vert3Val;
}

glm::vec3 ComputeBarycentricCoords(const Triangle& triangle, const glm::vec3& point)
{
	// Barycentric coordinates fulfill equation: point = [ v1; v2; v3 ] * barycentricCoords
	glm::mat3 vertexMat = glm::mat3(triangle.v0, triangle.v1, triangle.v2);	// Initialize columns with triangle verticies.
	glm::mat3 inverseVertexMat = glm::inverse(vertexMat);

	return inverseVertexMat * point;
}

glm::vec3 ComputeBarycentricCoords(const Triangle& triangle, const glm::vec3& norm, float voxelX, float voxelY, float voxelZ)
{
	glm::vec3 voxel = glm::vec3(voxelX, voxelY, voxelZ);

	// Triangle plane coefficients A,B,C are equal to triangleNormal.
	// We must compute D coefficient first.
	// Ax + By + Cz + D = 0, where (x,y,z) is one of triangle points.
	// D = -( n.x * v0.x + n.y * v0.y + n.z * v0.z ), where n is triangleNormal. 
	float coeffD = -glm::dot(triangle.v0, norm);

	// Our goal is to find point p such that p + k*n = (voxelX, voxelY, voxelZ).
	float k = (glm::dot(voxel, norm) + coeffD) / (norm.x * norm.x + norm.y * norm.y + norm.z * norm.z);
	glm::vec3 point = voxel - k * norm;

	return ComputeBarycentricCoords(triangle, point);
}