#include "voxelizer.h"

using namespace std;
using namespace trimesh;

void voxelize_partition(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, float unitlength, VoxelData** data, size_t &nfilled){
	// clear partition
	memset(*data,0,(morton_end-morton_start)*sizeof(VoxelData));
	// compute partition min and max in grid coords
	AABox<ivec3> p_bbox_grid;
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
		AABox<vec3> t_bbox_world;
		computeBoundingBox(t.v0,t.v1,t.v2,t_bbox_world);
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

		// construct test objects (sphere, cilinders, planes)
		Sphere sphere0 = Sphere(t.v0,radius);
		Sphere sphere1 = Sphere(t.v1,radius);
		Sphere sphere2 = Sphere(t.v2,radius);
		Cylinder cyl0 = Cylinder(t.v0,t.v1,radius);
		Cylinder cyl1 = Cylinder(t.v1,t.v2,radius);
		Cylinder cyl2 = Cylinder(t.v2,t.v0,radius);
		Plane S = Plane(t.v0,t.v1,t.v2);

		// test possible grid boxes for overlap
		for(int x = t_bbox_grid.min[0]; x <= t_bbox_grid.max[0]; x++){
			for(int y = t_bbox_grid.min[1]; y <= t_bbox_grid.max[1]; y++){
				for(int z = t_bbox_grid.min[2]; z <= t_bbox_grid.max[2]; z++){
					uint64_t index = mortonEncode(z,y,x);

					assert(index-morton_start < (morton_end-morton_start));
					
					if((*data)[index-morton_start].filled){ continue; } // already marked, continue

					vec3 middle_point = vec3((x+0.5f)*unitlength,(y+0.5f)*unitlength,(z+0.5f)*unitlength);
					
					// TEST 1: spheres in vertices
					if( isPointInSphere(middle_point,sphere0) || 
						isPointInSphere(middle_point,sphere1) || 
						isPointInSphere(middle_point,sphere2)){
#ifdef BINARY_VOXELIZATION
								(*data)[index-morton_start] = VoxelData(true);
#else
								(*data)[index-morton_start] = VoxelData(true, t.normal);
#endif
							nfilled++;
							continue;
					}
					// TEST 2: cylinders on edges
					if( isPointinCylinder(middle_point,cyl0) ||
						isPointinCylinder(middle_point,cyl1) ||
						isPointinCylinder(middle_point,cyl2)){
#ifdef BINARY_VOXELIZATION
								(*data)[index-morton_start] = VoxelData(true);
#else
								(*data)[index-morton_start] = VoxelData(true, t.normal);
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
						float s2 = (S.normal CROSS (t.v2-t.v1)) DOT (middle_point-t.v1); // (normal of E2) DOT (vector middlepoint->point on surf)
						float s3 = (S.normal CROSS (t.v0-t.v2)) DOT (middle_point-t.v2); // (normal of E3) DOT (vector middlepoint->point on surf)
						if( ((s1<=0) == (s2<=0)) && ((s2<=0) == (s3<=0)) ){
#ifdef BINARY_VOXELIZATION
								(*data)[index-morton_start] = VoxelData(true);
#else
								(*data)[index-morton_start] = VoxelData(true, t.normal);
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
