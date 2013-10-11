#include "voxelizer.h"

using namespace std;
using namespace trimesh;

// Implementation of http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.12.6294 (Huang et al.)
// Adapted for mortoncode -based subgrids
// by Jeroen Baert - jeroen.baert@cs.kuleuven.be

#define X 0
#define Y 1
#define Z 2

#ifdef BINARY_VOXELIZATION
void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled) {
	for(size_t i = 0; i < (morton_end-morton_start); i++){
		voxels[i] = EMPTY_VOXEL;
	}
#else
void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled) {
	for(size_t i = 0; i < (morton_end-morton_start); i++){
		voxels[i] = EMPTY_VOXEL;
	}
	voxel_data.clear();
#endif
	// compute partition min and max in grid coords
	AABox<uivec3> p_bbox_grid;
	mortonDecode(morton_start, p_bbox_grid.min[2], p_bbox_grid.min[1], p_bbox_grid.min[0]);
	mortonDecode(morton_end-1, p_bbox_grid.max[2], p_bbox_grid.max[1], p_bbox_grid.max[0]);
	// misc calc
	float unit_div = 1.0f / unitlength;
	float radius = unitlength / 2.0f;

	// voxelize every triangle
	while(reader.hasNext()) {
		// read triangle
		Triangle t;

		algo_timer.stop(); io_timer_in.start();
		reader.getTriangle(t);
		io_timer_in.stop(); algo_timer.start();

		// compute triangle bbox in world and grid
		AABox<vec3> t_bbox_world = computeBoundingBox(t.v0,t.v1,t.v2);
		AABox<uivec3> t_bbox_grid;
		t_bbox_grid.min[0] = (unsigned int) (t_bbox_world.min[0] * unit_div);
		t_bbox_grid.min[1] = (unsigned int) (t_bbox_world.min[1] * unit_div);
		t_bbox_grid.min[2] = (unsigned int) (t_bbox_world.min[2] * unit_div);
		t_bbox_grid.max[0] = (unsigned int) (t_bbox_world.max[0] * unit_div);
		t_bbox_grid.max[1] = (unsigned int) (t_bbox_world.max[1] * unit_div);
		t_bbox_grid.max[2] = (unsigned int) (t_bbox_world.max[2] * unit_div);

		// clamp
		t_bbox_grid.min[0]  = clampval<unsigned int>(t_bbox_grid.min[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.min[1]  = clampval<unsigned int>(t_bbox_grid.min[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.min[2]  = clampval<unsigned int>(t_bbox_grid.min[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);
		t_bbox_grid.max[0]  = clampval<unsigned int>(t_bbox_grid.max[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.max[1]  = clampval<unsigned int>(t_bbox_grid.max[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.max[2]  = clampval<unsigned int>(t_bbox_grid.max[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);

		// construct test objects (sphere, cilinders, planes)
		Sphere sphere0 = Sphere(t.v0,radius);
		Sphere sphere1 = Sphere(t.v1,radius);
		Sphere sphere2 = Sphere(t.v2,radius);
		Cylinder cyl0 = Cylinder(t.v0,t.v1,radius);
		Cylinder cyl1 = Cylinder(t.v1,t.v2,radius);
		Cylinder cyl2 = Cylinder(t.v2,t.v0,radius);
		Plane S = Plane(t.v0,t.v1,t.v2);

		// test possible grid boxes for overlap
		for(unsigned int x = t_bbox_grid.min[0]; x <= t_bbox_grid.max[0]; x++){
			for(unsigned int y = t_bbox_grid.min[1]; y <= t_bbox_grid.max[1]; y++){
				for(unsigned int z = t_bbox_grid.min[2]; z <= t_bbox_grid.max[2]; z++){
					uint64_t index = mortonEncode_LUT(z,y,x);

					assert(index-morton_start < (morton_end-morton_start));
					
					if(! voxels[index-morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue

					vec3 middle_point = vec3((x+0.5f)*unitlength,(y+0.5f)*unitlength,(z+0.5f)*unitlength);
					
					// TEST 1: spheres in vertices
					if( isPointInSphere(middle_point,sphere0) || 
						isPointInSphere(middle_point,sphere1) || 
						isPointInSphere(middle_point,sphere2)){
#ifdef BINARY_VOXELIZATION
							voxels[index-morton_start] = true;
#else
							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
							voxels[index-morton_start] = voxel_data.size()-1;
#endif
							nfilled++;
							continue;
					}
					// TEST 2: cylinders on edges
					if( isPointinCylinder(middle_point,cyl0) ||
						isPointinCylinder(middle_point,cyl1) ||
						isPointinCylinder(middle_point,cyl2)){
#ifdef BINARY_VOXELIZATION
							voxels[index-morton_start] = true;
#else
							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
							voxels[index-morton_start] = voxel_data.size()-1;
#endif
							nfilled++;
							continue;
					}
					// TEST3 : using planes
					//let's find beta
					float b0 = abs( vec3(unitlength/2.0f,0.0f,0.0f) DOT S.normal);
					float b1 = abs( vec3(0.0f,unitlength/2.0f,0.0f) DOT S.normal);
					float b2 = abs( vec3(0.0f,0.0f,unitlength/2.0f) DOT S.normal);
					float cosbeta = max(b0,max(b1,b2))/(len(vec3(unitlength/2.0f,0.0f,0.0f))*len(S.normal));
					float tc = (unitlength/2.0f)*cosbeta;
					//construct G and H and check if point is between these planes
					if(isPointBetweenParallelPlanes(middle_point,Plane(S.normal, S.D+tc),Plane(S.normal,S.D-tc))){
						float s1 = (S.normal CROSS (t.v1-t.v0)) DOT (middle_point-t.v0); // (normal of E1) DOT (vector middlepoint->point on surf)
						float s2 = (S.normal CROSS (t.v2-t.v1)) DOT (middle_point-t.v1);
						float s3 = (S.normal CROSS (t.v0-t.v2)) DOT (middle_point-t.v2);
						if( ((s1<=0) == (s2<=0)) && ((s2<=0) == (s3<=0)) ){
#ifdef BINARY_VOXELIZATION
							voxels[index-morton_start] = true;
#else
							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
							voxels[index-morton_start] = voxel_data.size()-1;
#endif
								nfilled++;
								continue;
						}
					}
				}
			}
		}
	}
}

#ifdef BINARY_VOXELIZATION
void voxelize_partition2(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled) {
#else
void voxelize_partition2(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled) {
	voxel_data.clear();
#endif
	for(size_t i = 0; i < (morton_end-morton_start); i++){
		voxels[i] = EMPTY_VOXEL;
	}

	// compute partition min and max in grid coords
	AABox<uivec3> p_bbox_grid;
	mortonDecode(morton_start, p_bbox_grid.min[2], p_bbox_grid.min[1], p_bbox_grid.min[0]);
	mortonDecode(morton_end-1, p_bbox_grid.max[2], p_bbox_grid.max[1], p_bbox_grid.max[0]);

	// COMMON PROPERTIES FOR ALL TRIANGLES
	float unit_div = 1.0f / unitlength;
	vec3 delta_p = vec3(unitlength,unitlength,unitlength);

	// voxelize every triangle
	while(reader.hasNext()) {
		// read triangle
		Triangle t;

		algo_timer.stop(); io_timer_in.start();
		reader.getTriangle(t);
		io_timer_in.stop(); algo_timer.start();

		// COMMON PROPERTIES FOR THE TRIANGLE
		vec3 e0 = t.v1 - t.v0;
		vec3 e1 = t.v2 - t.v1;
		vec3 e2 = t.v0 - t.v2;
		vec3 to_normalize = (e0) CROSS (e1);
		vec3 n = normalize(to_normalize); // triangle normal
		// PLANE TEST PROPERTIES
		vec3 c = vec3(0.0f,0.0f,0.0f); // critical point
		if(n[X] > 0) { c[X] = unitlength;} 
		if(n[Y] > 0) { c[Y] = unitlength;} 
		if(n[Z] > 0) { c[Z] = unitlength;} 
		float d1 = n DOT (c - t.v0); 
		float d2 = n DOT ((delta_p - c) - t.v0);
		// PROJECTION TEST PROPERTIES
		// XY plane
		vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]);
		vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]);
		vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
		if(n[Z] < 0.0f) { 
			n_xy_e0 = -1.0f * n_xy_e0;
			n_xy_e1 = -1.0f * n_xy_e1;
			n_xy_e2 = -1.0f * n_xy_e2;
		}
		float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X],t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
		float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X],t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
		float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X],t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);

		// YZ plane
		vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]);
		vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]);
		vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
		if(n[X] < 0.0f) { 
			n_yz_e0 = -1.0f * n_yz_e0;
			n_yz_e1 = -1.0f * n_yz_e1;
			n_yz_e2 = -1.0f * n_yz_e2;
		}
		float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y],t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
		float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y],t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
		float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y],t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);

		// ZX plane
		vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]);
		vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]);
		vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
		if(n[Y] < 0.0f) { 
			n_zx_e0 = -1.0f * n_zx_e0;
			n_zx_e1 = -1.0f * n_zx_e1;
			n_zx_e2 = -1.0f * n_zx_e2;
		}
		float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z],t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
		float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z],t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
		float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z],t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);

		// compute triangle bbox in world and grid
		AABox<vec3> t_bbox_world = computeBoundingBox(t.v0,t.v1,t.v2);
		AABox<ivec3> t_bbox_grid;
		t_bbox_grid.min[0] = (int) (t_bbox_world.min[0] * unit_div);
		t_bbox_grid.min[1] = (int) (t_bbox_world.min[1] * unit_div);
		t_bbox_grid.min[2] = (int) (t_bbox_world.min[2] * unit_div);
		t_bbox_grid.max[0] = (int) (t_bbox_world.max[0] * unit_div);
		t_bbox_grid.max[1] = (int) (t_bbox_world.max[1] * unit_div);
		t_bbox_grid.max[2] = (int) (t_bbox_world.max[2] * unit_div);

		// clamp
		t_bbox_grid.min[0]  = clampval<int>(t_bbox_grid.min[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.min[1]  = clampval<int>(t_bbox_grid.min[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.min[2]  = clampval<int>(t_bbox_grid.min[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);
		t_bbox_grid.max[0]  = clampval<int>(t_bbox_grid.max[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.max[1]  = clampval<int>(t_bbox_grid.max[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.max[2]  = clampval<int>(t_bbox_grid.max[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);

		// test possible grid boxes for overlap
		for(int x = t_bbox_grid.min[0]; x <= t_bbox_grid.max[0]; x++){
			for(int y = t_bbox_grid.min[1]; y <= t_bbox_grid.max[1]; y++){
				for(int z = t_bbox_grid.min[2]; z <= t_bbox_grid.max[2]; z++){

					uint64_t index = mortonEncode_LUT(z,y,x);

					assert(index-morton_start < (morton_end-morton_start));

					if(! voxels[index-morton_start] == EMPTY_VOXEL){continue;} // already marked, continue

					// TRIANGLE PLANE THROUGH BOX TEST
					vec3 p = vec3(x*unitlength,y*unitlength,z*unitlength);
					float nDOTp = n DOT p;
					if( (nDOTp + d1) * (nDOTp + d2) > 0.0f ){continue;}

					// PROJECTION TESTS
					// XY
					vec2 p_xy = vec2(p[X],p[Y]);
					if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){continue;}
					if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){continue;}
					if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){continue;}

					// YZ
					vec2 p_yz = vec2(p[Y],p[Z]);
					if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){continue;}
					if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){continue;}
					if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){continue;}

					// XZ	
					vec2 p_zx = vec2(p[Z],p[X]);
					if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){continue;}
					if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){continue;}
					if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){continue;}

#ifdef BINARY_VOXELIZATION
					voxels[index-morton_start] = true;
#else
					voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
					voxels[index-morton_start] = voxel_data.size()-1;
#endif
					nfilled++;
					continue;

				}
			}
		}
	}
}