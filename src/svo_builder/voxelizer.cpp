#include "voxelizer.h"
#include "BarycentricCoords.h"

using namespace std;
using namespace glm;

#define X 0
#define Y 1
#define Z 2

// Implementation of algorithm from http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.12.6294 (Huang et al.)
// Adapted for mortoncode -based subgrids

#ifdef BINARY_VOXELIZATION
void voxelize_huang_method(TriReader &reader, const ::uint64_t morton_start, const ::uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled) {
	for (size_t i = 0; i < (morton_end - morton_start); i++){ voxels[i] = EMPTY_VOXEL; }
#else
void voxelize_huang_method(TriReader &reader, const ::uint64_t morton_start, const ::uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled) {
	for (size_t i = 0; i < (morton_end - morton_start); i++){ voxels[i] = EMPTY_VOXEL; }
	voxel_data.clear();
#endif
	// compute partition min and max in grid coords
	AABox<uivec3> p_bbox_grid;
	morton3D_64_decode(morton_start, (uint_fast32_t&) p_bbox_grid.min[2], (uint_fast32_t&) p_bbox_grid.min[1], (uint_fast32_t&) p_bbox_grid.min[0]);
	morton3D_64_decode(morton_end - 1, (uint_fast32_t&) p_bbox_grid.max[2], (uint_fast32_t&) p_bbox_grid.max[1], (uint_fast32_t&) p_bbox_grid.max[0]);
	// misc calc
	float unit_div = 1.0f / unitlength;
	float radius = unitlength / 2.0f;

	// voxelize every triangle
	while (reader.hasNext()) {
		// read triangle
		Triangle t;

		//		algo_timer.stop(); io_timer_in.start();
		reader.getTriangle(t);
		//	io_timer_in.stop(); algo_timer.start();

		// compute triangle bbox in world and grid
		AABox<vec3> t_bbox_world = computeBoundingBox(t.v0, t.v1, t.v2);
		AABox<uivec3> t_bbox_grid;
		t_bbox_grid.min[0] = static_cast<unsigned int>(t_bbox_world.min[0] * unit_div);
		t_bbox_grid.min[1] = static_cast<unsigned int>(t_bbox_world.min[1] * unit_div);
		t_bbox_grid.min[2] = static_cast<unsigned int>(t_bbox_world.min[2] * unit_div);
		t_bbox_grid.max[0] = static_cast<unsigned int>(t_bbox_world.max[0] * unit_div);
		t_bbox_grid.max[1] = static_cast<unsigned int>(t_bbox_world.max[1] * unit_div);
		t_bbox_grid.max[2] = static_cast<unsigned int>(t_bbox_world.max[2] * unit_div);

		// clamp
		t_bbox_grid.min[0] = clampval<unsigned int>(t_bbox_grid.min[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.min[1] = clampval<unsigned int>(t_bbox_grid.min[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.min[2] = clampval<unsigned int>(t_bbox_grid.min[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);
		t_bbox_grid.max[0] = clampval<unsigned int>(t_bbox_grid.max[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.max[1] = clampval<unsigned int>(t_bbox_grid.max[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.max[2] = clampval<unsigned int>(t_bbox_grid.max[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);

		// construct test objects (sphere, cilinders, planes)
		Sphere sphere0 = Sphere(t.v0, radius);
		Sphere sphere1 = Sphere(t.v1, radius);
		Sphere sphere2 = Sphere(t.v2, radius);
		Cylinder cyl0 = Cylinder(t.v0, t.v1, radius);
		Cylinder cyl1 = Cylinder(t.v1, t.v2, radius);
		Cylinder cyl2 = Cylinder(t.v2, t.v0, radius);
		Plane S = Plane(t.v0, t.v1, t.v2);

		// test possible grid boxes for overlap
		for (unsigned int x = t_bbox_grid.min[0]; x <= t_bbox_grid.max[0]; x++){
			for (unsigned int y = t_bbox_grid.min[1]; y <= t_bbox_grid.max[1]; y++){
				for (unsigned int z = t_bbox_grid.min[2]; z <= t_bbox_grid.max[2]; z++){
					::uint64_t index = morton3D_64_encode(z, y, x);

					assert(index - morton_start < (morton_end - morton_start));

					if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue

					vec3 middle_point = vec3((x + 0.5f)*unitlength, (y + 0.5f)*unitlength, (z + 0.5f)*unitlength);

					// TEST 1: spheres in vertices
					if (isPointInSphere(middle_point, sphere0) ||
						isPointInSphere(middle_point, sphere1) ||
						isPointInSphere(middle_point, sphere2)){
#ifdef BINARY_VOXELIZATION
						voxels[index - morton_start] = true;
#else
						voxel_data.push_back(VoxelData(index, t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
						voxels[index - morton_start] = voxel_data.size() - 1;
#endif
						nfilled++;
						continue;
					}
					// TEST 2: cylinders on edges
					if (isPointinCylinder(middle_point, cyl0) ||
						isPointinCylinder(middle_point, cyl1) ||
						isPointinCylinder(middle_point, cyl2)){
#ifdef BINARY_VOXELIZATION
						voxels[index - morton_start] = true;
#else
						voxel_data.push_back(VoxelData(index, t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
						voxels[index - morton_start] = voxel_data.size() - 1;
#endif
						nfilled++;
						continue;
					}
					// TEST3 : using planes
					//let's find beta
					float halfunit = unitlength / 2.0f;
					float b0 = abs(dot(vec3(halfunit, 0.0f, 0.0f),S.normal));
					float b1 = abs(dot(vec3(0.0f, halfunit, 0.0f),S.normal));
					float b2 = abs(dot(vec3(0.0f, 0.0f, halfunit),S.normal));
					float cosbeta = std::max(b0, std::max(b1, b2)) / (length(vec3(halfunit, 0.0f, 0.0f))*length(S.normal));
					float tc = (unitlength / 2.0f)*cosbeta;
					//construct G and H and check if point is between these planes
					if (isPointBetweenParallelPlanes(middle_point, Plane(S.normal, S.D + tc), Plane(S.normal, S.D - tc))){
						float s1 = dot(cross(S.normal,t.v1-t.v0), middle_point-t.v0); // (normal of E1) DOT (vector middlepoint->point on surf)
						float s2 = dot(cross(S.normal,t.v2-t.v1), middle_point-t.v1);
						float s3 = dot(cross(S.normal,t.v0-t.v2), middle_point-t.v2);
						if (((s1 <= 0) == (s2 <= 0)) && ((s2 <= 0) == (s3 <= 0))){
#ifdef BINARY_VOXELIZATION
							voxels[index - morton_start] = true;
#else
							voxel_data.push_back(VoxelData(index, t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
							voxels[index - morton_start] = voxel_data.size() - 1;
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

// Implementation of algorithm from http://research.michael-schwarz.com/publ/2010/vox/ (Schwarz & Seidel)
// Adapted for mortoncode -based subgrids

#ifdef BINARY_VOXELIZATION
void voxelize_schwarz_method(TriReader &reader, const ::uint64_t morton_start, const ::uint64_t morton_end, const float unitlength, char* voxels, vector<::uint64_t> &data, float sparseness_limit, bool &use_data, size_t &nfilled) {
#else
void voxelize_schwarz_method(TriReader &reader, const ::uint64_t morton_start, const ::uint64_t morton_end, const float unitlength, char* voxels, vector<VoxelData> &data, float sparseness_limit, bool &use_data, size_t &nfilled) {
#endif
	vox_algo_timer.start();
	memset(voxels, EMPTY_VOXEL, (morton_end - morton_start)*sizeof(char));
	data.clear();

	// compute partition min and max in grid coords
	AABox<uivec3> p_bbox_grid;
	morton3D_64_decode(morton_start, (uint_fast32_t&) p_bbox_grid.min[0], (uint_fast32_t&) p_bbox_grid.min[1], (uint_fast32_t&) p_bbox_grid.min[2]); // note: flipped inputs here 0, 1, 2 vs 2, 1, 0
	morton3D_64_decode(morton_end - 1, (uint_fast32_t&) p_bbox_grid.max[0], (uint_fast32_t&) p_bbox_grid.max[1], (uint_fast32_t&) p_bbox_grid.max[2]); // note: flipped inputs here 0, 1, 2 vs 2, 1, 0

	// compute maximum grow size for data array
#ifdef BINARY_VOXELIZATION
	size_t data_max_items;
	if (use_data){
		::uint64_t max_bytes_data = (::uint64_t) (((morton_end - morton_start)*sizeof(char)) * sparseness_limit);

		data_max_items = max_bytes_data / sizeof(::uint64_t);
		data_max_items = max_bytes_data / sizeof(VoxelData);
	}
#endif

	// COMMON PROPERTIES FOR ALL TRIANGLES
	float unit_div = 1.0f / unitlength;
	vec3 delta_p = vec3(unitlength, unitlength, unitlength);

	// voxelize every triangle
	while (reader.hasNext()) {
		// read triangle
		Triangle t;

		vox_algo_timer.stop(); vox_io_in_timer.start();
		reader.getTriangle(t);
		vox_io_in_timer.stop(); vox_algo_timer.start();

#ifdef BINARY_VOXELIZATION
		if (use_data){
			if (data.size() > data_max_items){
				if (verbose){
					cout << "Sparseness optimization side-array overflowed, reverting to slower voxelization." << endl;
					cout << data.size() << " > " << data_max_items << endl;
				}
				use_data = false;
			}
		}
#endif

		// compute triangle bbox in world and grid
		AABox<vec3> t_bbox_world = computeBoundingBox(t.v0, t.v1, t.v2);
		AABox<ivec3> t_bbox_grid;
		t_bbox_grid.min[0] = static_cast<int>(t_bbox_world.min[0] * unit_div);
		t_bbox_grid.min[1] = static_cast<int>(t_bbox_world.min[1] * unit_div);
		t_bbox_grid.min[2] = static_cast<int>(t_bbox_world.min[2] * unit_div);
		t_bbox_grid.max[0] = static_cast<int>(t_bbox_world.max[0] * unit_div);
		t_bbox_grid.max[1] = static_cast<int>(t_bbox_world.max[1] * unit_div);
		t_bbox_grid.max[2] = static_cast<int>(t_bbox_world.max[2] * unit_div);

		// clamp
		t_bbox_grid.min[0] = clampval<int>(t_bbox_grid.min[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.min[1] = clampval<int>(t_bbox_grid.min[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.min[2] = clampval<int>(t_bbox_grid.min[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);
		t_bbox_grid.max[0] = clampval<int>(t_bbox_grid.max[0], p_bbox_grid.min[0], p_bbox_grid.max[0]);
		t_bbox_grid.max[1] = clampval<int>(t_bbox_grid.max[1], p_bbox_grid.min[1], p_bbox_grid.max[1]);
		t_bbox_grid.max[2] = clampval<int>(t_bbox_grid.max[2], p_bbox_grid.min[2], p_bbox_grid.max[2]);

		// COMMON PROPERTIES FOR THE TRIANGLE
		vec3 e0 = t.v1 - t.v0;
		vec3 e1 = t.v2 - t.v1;
		vec3 e2 = t.v0 - t.v2;
		vec3 n = normalize(cross(e0,e1)); // triangle normal
		// PLANE TEST PROPERTIES
		vec3 c = vec3(0.0f, 0.0f, 0.0f); // critical point
		if (n[X] > 0) { c[X] = unitlength; }
		if (n[Y] > 0) { c[Y] = unitlength; }
		if (n[Z] > 0) { c[Z] = unitlength; }
		float d1 = dot(n,c-t.v0);
		float d2 = dot(n,(delta_p - c) - t.v0);
		// PROJECTION TEST PROPERTIES
		// XY plane
		vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]);
		vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]);
		vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
		if (n[Z] < 0.0f) {
			n_xy_e0 = -1.0f * n_xy_e0;
			n_xy_e1 = -1.0f * n_xy_e1;
			n_xy_e2 = -1.0f * n_xy_e2;
		}
		float d_xy_e0 = (-1.0f * dot(n_xy_e0,vec2(t.v0[X], t.v0[Y]))) + std::max(0.0f, unitlength*n_xy_e0[0]) + std::max(0.0f, unitlength*n_xy_e0[1]);
		float d_xy_e1 = (-1.0f * dot(n_xy_e1,vec2(t.v1[X], t.v1[Y]))) + std::max(0.0f, unitlength*n_xy_e1[0]) + std::max(0.0f, unitlength*n_xy_e1[1]);
		float d_xy_e2 = (-1.0f * dot(n_xy_e2,vec2(t.v2[X], t.v2[Y]))) + std::max(0.0f, unitlength*n_xy_e2[0]) + std::max(0.0f, unitlength*n_xy_e2[1]);
		// YZ plane
		vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]);
		vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]);
		vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
		if (n[X] < 0.0f) {
			n_yz_e0 = -1.0f * n_yz_e0;
			n_yz_e1 = -1.0f * n_yz_e1;
			n_yz_e2 = -1.0f * n_yz_e2;
		}
		float d_yz_e0 = (-1.0f * dot(n_yz_e0,vec2(t.v0[Y], t.v0[Z]))) + std::max(0.0f, unitlength*n_yz_e0[0]) + std::max(0.0f, unitlength*n_yz_e0[1]);
		float d_yz_e1 = (-1.0f * dot(n_yz_e1,vec2(t.v1[Y], t.v1[Z]))) + std::max(0.0f, unitlength*n_yz_e1[0]) + std::max(0.0f, unitlength*n_yz_e1[1]);
		float d_yz_e2 = (-1.0f * dot(n_yz_e2,vec2(t.v2[Y], t.v2[Z]))) + std::max(0.0f, unitlength*n_yz_e2[0]) + std::max(0.0f, unitlength*n_yz_e2[1]);
		// ZX plane
		vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]);
		vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]);
		vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
		if (n[Y] < 0.0f) {
			n_zx_e0 = -1.0f * n_zx_e0;
			n_zx_e1 = -1.0f * n_zx_e1;
			n_zx_e2 = -1.0f * n_zx_e2;
		}
		float d_xz_e0 = (-1.0f * dot(n_zx_e0,vec2(t.v0[Z], t.v0[X]))) + std::max(0.0f, unitlength*n_zx_e0[0]) + std::max(0.0f, unitlength*n_zx_e0[1]);
		float d_xz_e1 = (-1.0f * dot(n_zx_e1,vec2(t.v1[Z], t.v1[X]))) + std::max(0.0f, unitlength*n_zx_e1[0]) + std::max(0.0f, unitlength*n_zx_e1[1]);
		float d_xz_e2 = (-1.0f * dot(n_zx_e2,vec2(t.v2[Z], t.v2[X]))) + std::max(0.0f, unitlength*n_zx_e2[0]) + std::max(0.0f, unitlength*n_zx_e2[1]);

		// test possible grid boxes for overlap
		for (int x = t_bbox_grid.min[0]; x <= t_bbox_grid.max[0]; x++){
			for (int y = t_bbox_grid.min[1]; y <= t_bbox_grid.max[1]; y++){
				for (int z = t_bbox_grid.min[2]; z <= t_bbox_grid.max[2]; z++){

					::uint64_t index = morton3D_64_encode(x, y, z);

					if (voxels[index - morton_start] == FULL_VOXEL){ continue; } // already marked, continue

					// TRIANGLE PLANE THROUGH BOX TEST
					vec3 p = vec3(x*unitlength, y*unitlength, z*unitlength);
					float nDOTp = dot(n,p);
					if ((nDOTp + d1) * (nDOTp + d2) > 0.0f){ continue; }

					// PROJECTION TESTS
					// XY
					vec2 p_xy = vec2(p[X], p[Y]);
					if ((dot(n_xy_e0,p_xy) + d_xy_e0) < 0.0f){ continue; }
					if ((dot(n_xy_e1,p_xy) + d_xy_e1) < 0.0f){ continue; }
					if ((dot(n_xy_e2,p_xy) + d_xy_e2) < 0.0f){ continue; }

					// YZ
					vec2 p_yz = vec2(p[Y], p[Z]);
					if ((dot(n_yz_e0,p_yz) + d_yz_e0) < 0.0f){ continue; }
					if ((dot(n_yz_e1,p_yz) + d_yz_e1) < 0.0f){ continue; }
					if ((dot(n_yz_e2,p_yz) + d_yz_e2) < 0.0f){ continue; }

					// XZ	
					vec2 p_zx = vec2(p[Z], p[X]);
					if ((dot(n_zx_e0,p_zx) + d_xz_e0) < 0.0f){ continue; }
					if ((dot(n_zx_e1,p_zx) + d_xz_e1) < 0.0f){ continue; }
					if ((dot(n_zx_e2,p_zx) + d_xz_e2) < 0.0f){ continue; }

#ifdef BINARY_VOXELIZATION
					voxels[index - morton_start] = FULL_VOXEL;
					if (use_data){ data.push_back(index); }
#else
					voxels[index - morton_start] = FULL_VOXEL;

					glm::vec3 barycentric = ComputeBarycentricCoords( t, n, x / unit_div, y / unit_div, z / unit_div );
					glm::vec3 voxelColor = InterpolateValue( barycentric, t.v0_color, t.v1_color, t.v2_color );
					//glm::vec3 voxelColor = average3Vec( t.v0_color, t.v1_color, t.v2_color );

					data.push_back(VoxelData(index, t.normal, voxelColor)); // we ignore data limits for colored voxelization
#endif
					nfilled++;
					continue;
				}
			}
		}
	}
}

//#ifdef BINARY_VOXELIZATION
//void voxelize_partition3(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, char* voxels, vector<uint64_t> &data, float sparseness_limit, bool &use_data, size_t &nfilled){
//	vox_algo_timer.start();
//	memset(voxels, EMPTY_VOXEL, (morton_end - morton_start)*sizeof(char));
//	data.clear();
//#else
//void voxelize_partition3(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled) {
//	voxel_data.clear();
//	memset(voxels, EMPTY_VOXEL, (morton_end - morton_start)*sizeof(size_t));
//#endif
//	// compute partition min and max in grid coords
//	AABox<uivec3> p_bbox_grid;
//	mortonDecode(morton_start, p_bbox_grid.min[2], p_bbox_grid.min[1], p_bbox_grid.min[0]);
//	mortonDecode(morton_end - 1, p_bbox_grid.max[2], p_bbox_grid.max[1], p_bbox_grid.max[0]);
//
//	// COMMON PROPERTIES FOR ALL TRIANGLES
//	float unit_div = 1.0f / unitlength;
//	vec3 delta_p = vec3(unitlength, unitlength, unitlength);
//
//	// compute maximum grow size for data array
//	size_t data_max_items;
//	if (use_data){
//		uint64_t max_bytes_data = ((morton_end - morton_start)*sizeof(char)) * sparseness_limit;
//		data_max_items = max_bytes_data / sizeof(uint64_t);
//	}
//
//	// voxelize every triangle
//	while (reader.hasNext()) {
//		// read triangle
//		Triangle t;
//
//		vox_algo_timer.stop(); vox_io_in_timer.start();
//		reader.getTriangle(t);
//		vox_io_in_timer.stop(); vox_algo_timer.start();
//
//		if (use_data){
//			if (data.size() > data_max_items){
//				cout << "Data side-array overflowed, reverting to normal voxelization." << endl;
//				cout << data.size() << " > " << data_max_items << endl;
//				use_data = false;
//			}
//		}
//
//		// compute triangle bbox in world and grid
//		AABox<vec3> t_bbox_world = computeBoundingBox(t.v0, t.v1, t.v2);
//		AABox<ivec3> t_bbox_grid;
//		t_bbox_grid.min[X] = (int)(t_bbox_world.min[X] * unit_div);
//		t_bbox_grid.min[Y] = (int)(t_bbox_world.min[Y] * unit_div);
//		t_bbox_grid.min[Z] = (int)(t_bbox_world.min[Z] * unit_div);
//		t_bbox_grid.max[X] = (int)(t_bbox_world.max[X] * unit_div);
//		t_bbox_grid.max[Y] = (int)(t_bbox_world.max[Y] * unit_div);
//		t_bbox_grid.max[Z] = (int)(t_bbox_world.max[Z] * unit_div);
//		// clamp
//		t_bbox_grid.min[X] = clampval<int>(t_bbox_grid.min[X], p_bbox_grid.min[X], p_bbox_grid.max[X]);
//		t_bbox_grid.min[Y] = clampval<int>(t_bbox_grid.min[Y], p_bbox_grid.min[Y], p_bbox_grid.max[Y]);
//		t_bbox_grid.min[Z] = clampval<int>(t_bbox_grid.min[Z], p_bbox_grid.min[Z], p_bbox_grid.max[Z]);
//		t_bbox_grid.max[X] = clampval<int>(t_bbox_grid.max[X], p_bbox_grid.min[X], p_bbox_grid.max[X]);
//		t_bbox_grid.max[Y] = clampval<int>(t_bbox_grid.max[Y], p_bbox_grid.min[Y], p_bbox_grid.max[Y]);
//		t_bbox_grid.max[Z] = clampval<int>(t_bbox_grid.max[Z], p_bbox_grid.min[Z], p_bbox_grid.max[Z]);
//
//		// There are 9 cases:
//		// 3 axes          x 1D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 2 directions
//		// 3 planes        x 2D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 1 direction
//		// 3 dominant axes x 3D Bounding Boxes: triangle bbox is of variable size
//
//		// Measure bbox thickness to find correct case
//		ivec3 one_thick = ivec3(0, 0, 0);
//		if (t_bbox_grid.min[X] == t_bbox_grid.max[X]) { one_thick[X] = 1; }
//		if (t_bbox_grid.min[Y] == t_bbox_grid.max[Y]) { one_thick[Y] = 1; }
//		if (t_bbox_grid.min[Z] == t_bbox_grid.max[Z]) { one_thick[Z] = 1; }
//
//		// CASE 1-3
//		// 3 axes x 1D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 2 directions
//		// TESTS: All voxels can be accepted without further test
//		if (one_thick.sum() >= 2){
//			for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//				for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//					for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//						uint64_t index = mortonEncode_LUT(t_bbox_grid.min[Z], t_bbox_grid.min[Y], t_bbox_grid.min[X]);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = 1;
//						if (use_data){data.push_back(index);}
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++;
//						continue;
//					}
//				}
//			}
//			continue; // go to next triangle
//		}
//
//		// COMMON PROPERTIES FOR THE TRIANGLE
//		vec3 e0 = t.v1 - t.v0;
//		vec3 e1 = t.v2 - t.v1;
//		vec3 e2 = t.v0 - t.v2;
//		vec3 to_normalize = (e0)CROSS(e1);
//		vec3 n = normalize(to_normalize); // triangle normal
//
//		// CASE 4-6:
//		// 3 planes x 2D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 1 direction
//		if (one_thick.sum() == 1) {
//			if (one_thick[X] == 1){ // 1 voxel thick in X direction - so 2D overlap test in YZ plane
//				// YZ plane projection test setup
//				vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]); vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]); vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
//				if (n[X] < 0.0f) { n_yz_e0 = -1.0f * n_yz_e0; n_yz_e1 = -1.0f * n_yz_e1; n_yz_e2 = -1.0f * n_yz_e2; }
//				float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y], t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
//				float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y], t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
//				float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y], t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);
//				// Test bbox projection in YZ plane
//				for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//					for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//						uint64_t index = mortonEncode_LUT(z, y, t_bbox_grid.min[X]);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//						// YZ PROJECTION TEST
//						vec2 p_yz = vec2(y*unitlength, z*unitlength);
//						if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){ continue; }
//						if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){ continue; }
//						if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = 1;
//						if (use_data){ data.push_back(index); }
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++; continue;
//					}
//				}
//				continue; // go to next triangle
//			}
//			else if (one_thick[Y] == 1) { // 1 voxel thick in Y direction - so 2D overlap test in ZX plane
//				// ZX plane projection test setup
//				vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]); vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]); vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
//				if (n[Y] < 0.0f) { n_zx_e0 = -1.0f * n_zx_e0; n_zx_e1 = -1.0f * n_zx_e1; n_zx_e2 = -1.0f * n_zx_e2; }
//				float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z], t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
//				float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z], t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
//				float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z], t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);
//				// Test bbox projection in ZX plane
//				for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//					for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//						uint64_t index = mortonEncode_LUT(z, t_bbox_grid.min[Y], x);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//						// XZ PROJECTION TEST
//						vec2 p_zx = vec2(z*unitlength, x*unitlength);
//						if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){ continue; }
//						if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){ continue; }
//						if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = 1;
//						if (use_data){ data.push_back(index); }
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++; continue;
//					}
//				}
//				continue; // go to next triangle
//			}
//			else if (one_thick[Z] == 1) { // 1 voxel thick in Z direction - so 2D overlap test in XY plane
//				// XY plane projection test setup
//				vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]); vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]); vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
//				if (n[Z] < 0.0f) { n_xy_e0 = -1.0f * n_xy_e0; n_xy_e1 = -1.0f * n_xy_e1; n_xy_e2 = -1.0f * n_xy_e2; }
//				float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X], t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
//				float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X], t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
//				float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X], t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);
//				// Test bbox projection in ZX plane
//				for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//					for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//						uint64_t index = mortonEncode_LUT(t_bbox_grid.min[Z], y, x);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//						// XY PROJECTION TEST
//						vec2 p_xy = vec2(x*unitlength, y*unitlength);
//						if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){ continue; }
//						if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){ continue; }
//						if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = 1;
//						if (use_data){ data.push_back(index); }
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++; continue;
//					}
//				}
//				continue; // go to next triangle
//			}
//			continue; // go to next triangle
//		}
//
//		// COMMON PROPERTIES FOR THE TRIANGLE
//		float d = (-1.0)*(n[X] * t.v0[X] + n[Y] * t.v0[Y] + n[Z] * t.v0[Z]); // d in plane equation
//		// Determine normal dominant axis
//		int dominant_axis = X;
//		if (n[Y] > n[dominant_axis]){ dominant_axis = Y; }
//		if (n[Z] > n[dominant_axis]){ dominant_axis = Z; }
//
//		// CASE 7-9
//		// 3 dominant axes x 3D Bounding Boxes: triangle bbox is of variable size
//		// Test: Find dominant axis and test that
//
//		if (one_thick.sum() == 0) {
//			// PLANE TEST PROPERTIES
//			vec3 c = vec3(); // critical point
//			if (n[X] > 0) { c[X] = unitlength; }
//			if (n[Y] > 0) { c[Y] = unitlength; }
//			if (n[Z] > 0) { c[Z] = unitlength; }
//			float d1 = n DOT(c - t.v0);
//			float d2 = n DOT((delta_p - c) - t.v0);
//			// PROJECTION TEST PROPERTIES
//			// XY plane
//			vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]);
//			vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]);
//			vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
//			if (n[Z] < 0.0f) {
//				n_xy_e0 = -1.0f * n_xy_e0;
//				n_xy_e1 = -1.0f * n_xy_e1;
//				n_xy_e2 = -1.0f * n_xy_e2;
//			}
//			float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X], t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
//			float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X], t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
//			float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X], t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);
//
//			// YZ plane
//			vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]);
//			vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]);
//			vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
//			if (n[X] < 0.0f) {
//				n_yz_e0 = -1.0f * n_yz_e0;
//				n_yz_e1 = -1.0f * n_yz_e1;
//				n_yz_e2 = -1.0f * n_yz_e2;
//			}
//			float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y], t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
//			float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y], t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
//			float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y], t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);
//
//			// ZX plane
//			vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]);
//			vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]);
//			vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
//			if (n[Y] < 0.0f) {
//				n_zx_e0 = -1.0f * n_zx_e0;
//				n_zx_e1 = -1.0f * n_zx_e1;
//				n_zx_e2 = -1.0f * n_zx_e2;
//			}
//			float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z], t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
//			float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z], t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
//			float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z], t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);
//
//			// test possible grid boxes for overlap
//			for (int x = t_bbox_grid.min[0]; x <= t_bbox_grid.max[0]; x++){
//				for (int y = t_bbox_grid.min[1]; y <= t_bbox_grid.max[1]; y++){
//					for (int z = t_bbox_grid.min[2]; z <= t_bbox_grid.max[2]; z++){
//
//						uint64_t index = mortonEncode_LUT(z, y, x);
//
//						assert(index - morton_start < (morton_end - morton_start));
//
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//
//						// TRIANGLE PLANE THROUGH BOX TEST
//						vec3 p = vec3(x*unitlength, y*unitlength, z*unitlength);
//						float nDOTp = n DOT p;
//						if ((nDOTp + d1) * (nDOTp + d2) > 0.0f){ continue; }
//
//						// PROJECTION TESTS
//						// XY
//						vec2 p_xy = vec2(p[X], p[Y]);
//						if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){ continue; }
//						if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){ continue; }
//						if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){ continue; }
//
//						// YZ
//						vec2 p_yz = vec2(p[Y], p[Z]);
//						if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){ continue; }
//						if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){ continue; }
//						if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){ continue; }
//
//						// XZ	
//						vec2 p_zx = vec2(p[Z], p[X]);
//						if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){ continue; }
//						if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){ continue; }
//						if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){ continue; }
//
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = 1;
//						if (use_data){ data.push_back(index); }
//#else
//						voxel_data.push_back(VoxelData(index,t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++;
//						continue;
//
//					}
//				}
//			}
//		}
//	}
//	vox_algo_timer.stop();
//}
//
//#ifdef BINARY_VOXELIZATION
//void voxelize_partition4(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, bool* voxels, size_t &nfilled) {
//#else
//void voxelize_partition4(TriReader &reader, const uint64_t morton_start, const uint64_t morton_end, const float unitlength, size_t* voxels, vector<VoxelData>& voxel_data, size_t &nfilled) {
//	voxel_data.clear();
//#endif
//	for (size_t i = 0; i < (morton_end - morton_start); i++){ voxels[i] = EMPTY_VOXEL; }
//
//	// compute partition min and max in grid coords
//	AABox<uivec3> p_bbox_grid;
//	mortonDecode(morton_start, p_bbox_grid.min[2], p_bbox_grid.min[1], p_bbox_grid.min[0]);
//	mortonDecode(morton_end - 1, p_bbox_grid.max[2], p_bbox_grid.max[1], p_bbox_grid.max[0]);
//
//	// COMMON PROPERTIES FOR ALL TRIANGLES
//	float unit_div = 1.0f / unitlength;
//	vec3 delta_p = vec3(unitlength, unitlength, unitlength);
//
//	// voxelize every triangle
//	while (reader.hasNext()) {
//		// read triangle
//		Triangle t;
//
//		//algo_timer.stop(); io_timer_in.start();
//		reader.getTriangle(t);
//		//io_timer_in.stop(); algo_timer.start();
//
//		// compute triangle bbox in world and grid
//		AABox<vec3> t_bbox_world = computeBoundingBox(t.v0, t.v1, t.v2);
//		AABox<ivec3> t_bbox_grid;
//		t_bbox_grid.min[X] = (int)(t_bbox_world.min[X] * unit_div);
//		t_bbox_grid.min[Y] = (int)(t_bbox_world.min[Y] * unit_div);
//		t_bbox_grid.min[Z] = (int)(t_bbox_world.min[Z] * unit_div);
//		t_bbox_grid.max[X] = (int)(t_bbox_world.max[X] * unit_div);
//		t_bbox_grid.max[Y] = (int)(t_bbox_world.max[Y] * unit_div);
//		t_bbox_grid.max[Z] = (int)(t_bbox_world.max[Z] * unit_div);
//		// clamp
//		t_bbox_grid.min[X] = clampval<int>(t_bbox_grid.min[X], p_bbox_grid.min[X], p_bbox_grid.max[X]);
//		t_bbox_grid.min[Y] = clampval<int>(t_bbox_grid.min[Y], p_bbox_grid.min[Y], p_bbox_grid.max[Y]);
//		t_bbox_grid.min[Z] = clampval<int>(t_bbox_grid.min[Z], p_bbox_grid.min[Z], p_bbox_grid.max[Z]);
//		t_bbox_grid.max[X] = clampval<int>(t_bbox_grid.max[X], p_bbox_grid.min[X], p_bbox_grid.max[X]);
//		t_bbox_grid.max[Y] = clampval<int>(t_bbox_grid.max[Y], p_bbox_grid.min[Y], p_bbox_grid.max[Y]);
//		t_bbox_grid.max[Z] = clampval<int>(t_bbox_grid.max[Z], p_bbox_grid.min[Z], p_bbox_grid.max[Z]);
//
//		// There are 9 cases:
//		// 3 axes          x 1D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 2 directions
//		// 3 planes        x 2D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 1 direction
//		// 3 dominant axes x 3D Bounding Boxes: triangle bbox is of variable size
//
//		// Measure bbox thickness to find correct case
//		ivec3 one_thick = ivec3(0, 0, 0);
//		if (t_bbox_grid.min[X] == t_bbox_grid.max[X]) { one_thick[X] = 1; }
//		if (t_bbox_grid.min[Y] == t_bbox_grid.max[Y]) { one_thick[Y] = 1; }
//		if (t_bbox_grid.min[Z] == t_bbox_grid.max[Z]) { one_thick[Z] = 1; }
//
//		// CASE 1-3
//		// 3 axes x 1D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 2 directions
//		// TESTS: All voxels can be accepted without further test
//		if (one_thick.sum() >= 2){
//			for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//				for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//					for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//						uint64_t index = mortonEncode_LUT(t_bbox_grid.min[Z], t_bbox_grid.min[Y], t_bbox_grid.min[X]);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = true;
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++;
//						continue;
//					}
//				}
//			}
//			continue; // go to next triangle
//		}
//
//		// COMMON PROPERTIES FOR THE TRIANGLE
//		vec3 e0 = t.v1 - t.v0;
//		vec3 e1 = t.v2 - t.v1;
//		vec3 e2 = t.v0 - t.v2;
//		vec3 to_normalize = (e0)CROSS(e1);
//		vec3 n = normalize(to_normalize); // triangle normal
//
//		// CASE 4-6:
//		// 3 planes x 2D Bounding Boxes: triangle bbox is only 1 voxel thick in at least 1 direction
//		if (one_thick.sum() == 1) {
//			if (one_thick[X] == 1){ // 1 voxel thick in X direction - so 2D overlap test in YZ plane
//				// YZ plane projection test setup
//				vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]); vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]); vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
//				if (n[X] < 0.0f) { n_yz_e0 = -1.0f * n_yz_e0; n_yz_e1 = -1.0f * n_yz_e1; n_yz_e2 = -1.0f * n_yz_e2; }
//				float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y], t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
//				float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y], t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
//				float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y], t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);
//				// Test bbox projection in YZ plane
//				for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//					for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//						uint64_t index = mortonEncode_LUT(z, y, t_bbox_grid.min[X]);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//						// YZ PROJECTION TEST
//						vec2 p_yz = vec2(y*unitlength, z*unitlength);
//						if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){ continue; }
//						if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){ continue; }
//						if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = true;
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++; continue;
//					}
//				}
//				continue; // go to next triangle
//			}
//			else if (one_thick[Y] == 1) { // 1 voxel thick in Y direction - so 2D overlap test in ZX plane
//				// ZX plane projection test setup
//				vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]); vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]); vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
//				if (n[Y] < 0.0f) { n_zx_e0 = -1.0f * n_zx_e0; n_zx_e1 = -1.0f * n_zx_e1; n_zx_e2 = -1.0f * n_zx_e2; }
//				float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z], t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
//				float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z], t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
//				float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z], t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);
//				// Test bbox projection in ZX plane
//				for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//					for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//						uint64_t index = mortonEncode_LUT(z, t_bbox_grid.min[Y], x);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//						// XZ PROJECTION TEST
//						vec2 p_zx = vec2(z*unitlength, x*unitlength);
//						if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){ continue; }
//						if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){ continue; }
//						if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = true;
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++; continue;
//					}
//				}
//				continue; // go to next triangle
//			}
//			else if (one_thick[Z] == 1) { // 1 voxel thick in Z direction - so 2D overlap test in XY plane
//				// XY plane projection test setup
//				vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]); vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]); vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
//				if (n[Z] < 0.0f) { n_xy_e0 = -1.0f * n_xy_e0; n_xy_e1 = -1.0f * n_xy_e1; n_xy_e2 = -1.0f * n_xy_e2; }
//				float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X], t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
//				float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X], t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
//				float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X], t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);
//				// Test bbox projection in ZX plane
//				for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//					for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//						uint64_t index = mortonEncode_LUT(t_bbox_grid.min[Z], y, x);
//						if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//						// XY PROJECTION TEST
//						vec2 p_xy = vec2(x*unitlength, y*unitlength);
//						if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){ continue; }
//						if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){ continue; }
//						if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//						voxels[index - morton_start] = true;
//#else
//						voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//						voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//						nfilled++; continue;
//					}
//				}
//				continue; // go to next triangle
//			}
//			continue; // go to next triangle
//		}
//
//		// COMMON PROPERTIES FOR THE TRIANGLE
//		float d = (-1.0)*(n[X] * t.v0[X] + n[Y] * t.v0[Y] + n[Z] * t.v0[Z]); // d in plane equation
//		// Determine normal dominant axis
//		int dominant_axis = X;
//		if (n[Y] > n[dominant_axis]){ dominant_axis = Y; }
//		if (n[Z] > n[dominant_axis]){ dominant_axis = Z; }
//
//		// CASE 7-9
//		// 3 dominant axes x 3D Bounding Boxes: triangle bbox is of variable size
//		// Test: Find dominant axis and test that
//
//		if (one_thick.sum() == 0) {
//			if (dominant_axis == X){
//				// YZ plane projection test setup
//				vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]); vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]); vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
//				if (n[X] < 0.0f) { n_yz_e0 = -1.0f * n_yz_e0; n_yz_e1 = -1.0f * n_yz_e1; n_yz_e2 = -1.0f * n_yz_e2; }
//				float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y], t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
//				float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y], t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
//				float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y], t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);
//				for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//					for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//						// YZ projection tests
//						vec2 p_yz = vec2(y*unitlength, z*unitlength);
//						if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){ continue; }
//						if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){ continue; }
//						if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){ continue; }
//
//						// Column test: Determine range of voxels in X direction
//						// (1) Determine min and max corners
//						vec2 min_corner = p_yz;
//						vec2 max_corner = p_yz + vec2(unitlength, unitlength);
//						if (n[Y] < 0) { swap(min_corner[0], max_corner[0]); }
//						if (n[Z] < 0) { swap(min_corner[1], max_corner[1]); }
//						// (2) Project corners on triangle plane (Equation: n_x*x + n_y*y + n_z*z + d = 0)
//						float x_min_world = (n[Y] * min_corner[0] + n[Z] * min_corner[1] + d) / (-1.0f * n[X]);
//						float x_max_world = (n[Y] * max_corner[0] + n[Z] * max_corner[1] + d) / (-1.0f * n[X]);
//						int x_min = x_min_world / unitlength;
//						int x_max = x_max_world / unitlength;
//						x_min = clampval<int>(x_min, p_bbox_grid.min[X], p_bbox_grid.max[X]);
//						x_max = clampval<int>(x_max, p_bbox_grid.min[X], p_bbox_grid.max[X]);
//
//						// special case if depth range is == 1
//						if (x_min == x_max){
//							uint64_t index = mortonEncode_LUT(z, y, x_min);
//							if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//#ifdef BINARY_VOXELIZATION
//							voxels[index - morton_start] = true;
//#else
//							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//							voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//							nfilled++; continue;
//						}
//
//						// otherwise also text XY and ZX overlap
//						// ZX plane projection test setup
//						vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]); vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]); vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
//						if (n[Y] < 0.0f) { n_zx_e0 = -1.0f * n_zx_e0; n_zx_e1 = -1.0f * n_zx_e1; n_zx_e2 = -1.0f * n_zx_e2; }
//						float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z], t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
//						float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z], t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
//						float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z], t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);
//						// XY plane projection test setup
//						vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]); vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]); vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
//						if (n[Z] < 0.0f) { n_xy_e0 = -1.0f * n_xy_e0; n_xy_e1 = -1.0f * n_xy_e1; n_xy_e2 = -1.0f * n_xy_e2; }
//						float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X], t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
//						float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X], t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
//						float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X], t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);
//						for (int x = x_min; x <= x_max; x++){
//							uint64_t index = mortonEncode_LUT(z, y, x);
//							if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//							// XY
//							vec2 p_xy = vec2(x*unitlength, y*unitlength);
//							if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){ continue; }
//							if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){ continue; }
//							if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){ continue; }
//							// XZ	
//							vec2 p_zx = vec2(z*unitlength, x*unitlength);
//							if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){ continue; }
//							if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){ continue; }
//							if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//							voxels[index - morton_start] = true;
//#else
//							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color, t.v1_color, t.v2_color)));
//							voxels[index - morton_start] = voxel_data.size() - 1;
//#endif
//							nfilled++;
//							continue;
//						}
//					}
//				}
//				continue; // go to next triangle
//			}
//			else if (dominant_axis == Y){
//				// ZX plane projection test setup
//				vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]); vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]); vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
//				if (n[Y] < 0.0f) { n_zx_e0 = -1.0f * n_zx_e0; n_zx_e1 = -1.0f * n_zx_e1; n_zx_e2 = -1.0f * n_zx_e2; }
//				float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z], t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
//				float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z], t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
//				float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z], t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);
//
//				for (int z = t_bbox_grid.min[Z]; z <= t_bbox_grid.max[Z]; z++){
//					for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//						// XZ	
//						vec2 p_zx = vec2(z*unitlength, x*unitlength);
//						if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){ continue; }
//						if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){ continue; }
//						if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){ continue; }
//
//						// Column test: Determine range of voxels in Y direction
//						// (1) Determine min and max corners
//						vec2 min_corner = p_zx;
//						vec2 max_corner = p_zx + vec2(unitlength, unitlength);
//						if (n[Z] < 0) { swap(min_corner[0], max_corner[0]); }
//						if (n[X] < 0) { swap(min_corner[1], max_corner[1]); }
//						// (2) Project corners on triangle plane (Equation: n_x*x + n_y*y + n_z*z + d = 0)
//						float y_min_world = (n[Z] * min_corner[0] + n[X] * min_corner[1] + d) / (-1.0f * n[Y]);
//						float y_max_world = (n[Z] * max_corner[0] + n[X] * max_corner[1] + d) / (-1.0f * n[Y]);
//						int y_min = y_min_world / unitlength;
//						int y_max = y_max_world / unitlength;
//						y_min = clampval<int>(y_min, p_bbox_grid.min[Y], p_bbox_grid.max[Y]);
//						y_max = clampval<int>(y_min, p_bbox_grid.min[Y], p_bbox_grid.max[Y]);
//
//						// special case if depth range is == 1
//						if (y_min == y_max){
//							uint64_t index = mortonEncode_LUT(z, y_min, x);
//							if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//#ifdef BINARY_VOXELIZATION
//							voxels[index - morton_start] = true;
//#else
//							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
//							voxels[index-morton_start] = voxel_data.size()-1;
//#endif
//							nfilled++; continue;
//						}
//
//						// otherwise also text XY and ZX overlap
//						// YZ plane projection test setup
//						vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]); vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]); vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
//						if (n[X] < 0.0f) { n_yz_e0 = -1.0f * n_yz_e0; n_yz_e1 = -1.0f * n_yz_e1; n_yz_e2 = -1.0f * n_yz_e2; }
//						float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y], t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
//						float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y], t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
//						float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y], t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);
//						// XY plane projection test setup
//						vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]); vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]); vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
//						if (n[Z] < 0.0f) { n_xy_e0 = -1.0f * n_xy_e0; n_xy_e1 = -1.0f * n_xy_e1; n_xy_e2 = -1.0f * n_xy_e2; }
//						float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X], t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
//						float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X], t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
//						float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X], t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);
//						for (int y = y_min; y <= y_max; y++){
//							uint64_t index = mortonEncode_LUT(z, y, x);
//							if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//							// YZ projection tests
//							vec2 p_yz = vec2(y*unitlength, z*unitlength);
//							if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){ continue; }
//							if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){ continue; }
//							if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){ continue; }
//							// XY
//							vec2 p_xy = vec2(x*unitlength, y*unitlength);
//							if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){ continue; }
//							if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){ continue; }
//							if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){ continue; }
//
//#ifdef BINARY_VOXELIZATION
//							voxels[index - morton_start] = true;
//#else
//							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
//							voxels[index-morton_start] = voxel_data.size()-1;
//#endif
//							nfilled++;
//							continue;
//						}
//					}
//				}
//				continue; // go to next triangle
//			}
//			else if (dominant_axis == Z){
//				// XY plane projection test setup
//				vec2 n_xy_e0 = vec2(-1.0f*e0[Y], e0[X]); vec2 n_xy_e1 = vec2(-1.0f*e1[Y], e1[X]); vec2 n_xy_e2 = vec2(-1.0f*e2[Y], e2[X]);
//				if (n[Z] < 0.0f) { n_xy_e0 = -1.0f * n_xy_e0; n_xy_e1 = -1.0f * n_xy_e1; n_xy_e2 = -1.0f * n_xy_e2; }
//				float d_xy_e0 = (-1.0f * (n_xy_e0 DOT vec2(t.v0[X], t.v0[Y]))) + max(0.0f, unitlength*n_xy_e0[0]) + max(0.0f, unitlength*n_xy_e0[1]);
//				float d_xy_e1 = (-1.0f * (n_xy_e1 DOT vec2(t.v1[X], t.v1[Y]))) + max(0.0f, unitlength*n_xy_e1[0]) + max(0.0f, unitlength*n_xy_e1[1]);
//				float d_xy_e2 = (-1.0f * (n_xy_e2 DOT vec2(t.v2[X], t.v2[Y]))) + max(0.0f, unitlength*n_xy_e2[0]) + max(0.0f, unitlength*n_xy_e2[1]);
//
//				for (int x = t_bbox_grid.min[X]; x <= t_bbox_grid.max[X]; x++){
//					for (int y = t_bbox_grid.min[Y]; y <= t_bbox_grid.max[Y]; y++){
//
//						// XY
//						vec2 p_xy = vec2(x*unitlength, y*unitlength);
//						if (((n_xy_e0 DOT p_xy) + d_xy_e0) < 0.0f){ continue; }
//						if (((n_xy_e1 DOT p_xy) + d_xy_e1) < 0.0f){ continue; }
//						if (((n_xy_e2 DOT p_xy) + d_xy_e2) < 0.0f){ continue; }
//
//						// Column test: Determine range of voxels in Z direction
//						// (1) Determine min and max corners
//						vec2 min_corner = p_xy;
//						vec2 max_corner = p_xy + vec2(unitlength, unitlength);
//						if (n[X] < 0) { swap(min_corner[0], max_corner[0]); }
//						if (n[Y] < 0) { swap(min_corner[1], max_corner[1]); }
//						// (2) Project corners on triangle plane (Equation: n_x*x + n_y*y + n_z*z + d = 0)
//						float z_min_world = (n[X] * min_corner[0] + n[Y] * min_corner[1] + d) / (-1.0f * n[Z]);
//						float z_max_world = (n[X] * max_corner[0] + n[Y] * max_corner[1] + d) / (-1.0f * n[Z]);
//						int z_min = z_min_world / unitlength;
//						int z_max = z_max_world / unitlength;
//						z_min = clampval<int>(z_min, p_bbox_grid.min[Z], p_bbox_grid.max[Z]);
//						z_max = clampval<int>(z_min, p_bbox_grid.min[Z], p_bbox_grid.max[Z]);
//
//						// special case if depth range is == 1
//						if (z_min == z_max){
//							uint64_t index = mortonEncode_LUT(z_min, y, x);
//							if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//#ifdef BINARY_VOXELIZATION
//							voxels[index - morton_start] = true;
//#else
//							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
//							voxels[index-morton_start] = voxel_data.size()-1;
//#endif
//							nfilled++; continue;
//						}
//
//						// ZX plane projection test setup
//						vec2 n_zx_e0 = vec2(-1.0f*e0[X], e0[Z]); vec2 n_zx_e1 = vec2(-1.0f*e1[X], e1[Z]); vec2 n_zx_e2 = vec2(-1.0f*e2[X], e2[Z]);
//						if (n[Y] < 0.0f) { n_zx_e0 = -1.0f * n_zx_e0; n_zx_e1 = -1.0f * n_zx_e1; n_zx_e2 = -1.0f * n_zx_e2; }
//						float d_xz_e0 = (-1.0f * (n_zx_e0 DOT vec2(t.v0[Z], t.v0[X]))) + max(0.0f, unitlength*n_zx_e0[0]) + max(0.0f, unitlength*n_zx_e0[1]);
//						float d_xz_e1 = (-1.0f * (n_zx_e1 DOT vec2(t.v1[Z], t.v1[X]))) + max(0.0f, unitlength*n_zx_e1[0]) + max(0.0f, unitlength*n_zx_e1[1]);
//						float d_xz_e2 = (-1.0f * (n_zx_e2 DOT vec2(t.v2[Z], t.v2[X]))) + max(0.0f, unitlength*n_zx_e2[0]) + max(0.0f, unitlength*n_zx_e2[1]);
//						// YZ plane projection test setup
//						vec2 n_yz_e0 = vec2(-1.0f*e0[Z], e0[Y]); vec2 n_yz_e1 = vec2(-1.0f*e1[Z], e1[Y]); vec2 n_yz_e2 = vec2(-1.0f*e2[Z], e2[Y]);
//						if (n[X] < 0.0f) { n_yz_e0 = -1.0f * n_yz_e0; n_yz_e1 = -1.0f * n_yz_e1; n_yz_e2 = -1.0f * n_yz_e2; }
//						float d_yz_e0 = (-1.0f * (n_yz_e0 DOT vec2(t.v0[Y], t.v0[Z]))) + max(0.0f, unitlength*n_yz_e0[0]) + max(0.0f, unitlength*n_yz_e0[1]);
//						float d_yz_e1 = (-1.0f * (n_yz_e1 DOT vec2(t.v1[Y], t.v1[Z]))) + max(0.0f, unitlength*n_yz_e1[0]) + max(0.0f, unitlength*n_yz_e1[1]);
//						float d_yz_e2 = (-1.0f * (n_yz_e2 DOT vec2(t.v2[Y], t.v2[Z]))) + max(0.0f, unitlength*n_yz_e2[0]) + max(0.0f, unitlength*n_yz_e2[1]);
//
//						for (int z = z_min; z <= z_max; z++){
//							uint64_t index = mortonEncode_LUT(z, y, x);
//							if (!voxels[index - morton_start] == EMPTY_VOXEL){ continue; } // already marked, continue
//
//							// YZ projection test
//							vec2 p_yz = vec2(y*unitlength, z*unitlength);
//							if (((n_yz_e0 DOT p_yz) + d_yz_e0) < 0.0f){ continue; }
//							if (((n_yz_e1 DOT p_yz) + d_yz_e1) < 0.0f){ continue; }
//							if (((n_yz_e2 DOT p_yz) + d_yz_e2) < 0.0f){ continue; }
//							// ZX projection test
//							vec2 p_zx = vec2(z*unitlength, x*unitlength);
//							if (((n_zx_e0 DOT p_zx) + d_xz_e0) < 0.0f){ continue; }
//							if (((n_zx_e1 DOT p_zx) + d_xz_e1) < 0.0f){ continue; }
//							if (((n_zx_e2 DOT p_zx) + d_xz_e2) < 0.0f){ continue; }
//#ifdef BINARY_VOXELIZATION
//							voxels[index - morton_start] = true;
//#else
//							voxel_data.push_back(VoxelData(t.normal, average3Vec(t.v0_color,t.v1_color,t.v2_color)));
//							voxels[index-morton_start] = voxel_data.size()-1;
//#endif
//							nfilled++;
//							continue;
//						}
//					}
//				}
//			}
//		}
//	}
//}