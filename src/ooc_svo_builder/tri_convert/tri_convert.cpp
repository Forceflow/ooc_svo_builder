#include <TriMesh.h>
#include <vector>
#include <string>
#include <sstream>
#include <tri_tools.h>
#include "util.h"

using namespace std;
using namespace trimesh;

#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// meshes - collect them
vector<TriMesh *> meshes;

void printInfo(){
	cout << "-------------------------------------------------------------" << endl;
#ifdef BINARY_VOXELIZATION
	cout << "Tri Converter 1.0 - BINARY VOXELIZATION"<< endl;
#else
	cout << "Tri Converter 1.0 - GEOMETRY+NORMALS"<< endl;
#endif
#ifdef _WIN32 || _WIN64
	cout << "Windows ";
#endif
#ifdef __linux__
	cout << "Linux ";
#endif
#ifdef ENVIRONMENT64
	cout << "64-bit version" << endl;
#endif
#ifdef ENVIRONMENT32
	cout << "32-bit version" << endl;
#endif
	cout << "Jeroen Baert - jeroen.baert@cs.kuleuven.be - www.forceflow.be" << endl;
	cout << "-------------------------------------------------------------" << endl << endl;
}

void printInvalid(){
	std::cout << "Not enough or invalid arguments, please try again.\n" << endl; 
	std::cout << "At the bare minimum, I need a path to a .PLY, .OBJ, .3DS, SM, RAY or .OFF file" << endl; 
	std::cout << "For Example: tri_convert.exe -f /home/jeroen/bunny.ply" << endl;
}

void parseParameters(int argc, char* argv[], string& filename){
	// Input argument validation
	if(argc<3){printInvalid(); exit(0);}
	for (int i = 1; i < argc; i++) {
			// parse filename
			if (string(argv[i]) == "-f") {
				filename = argv[i + 1]; i++;
			} else {
				printInvalid(); exit(0);
			}
	}
}

int main(int argc, char *argv[]){
	printInfo();

	// Parse parameters
	string filename = "";
	parseParameters(argc,argv,filename);

	// Read mesh
	cout << "Reading mesh from " << filename << endl;
	TriMesh *themesh = TriMesh::read(filename.c_str());
	themesh->need_faces();
	themesh->need_bbox();
	themesh->need_normals();
	meshes.push_back(themesh);
	AABox<vec3> mesh_bbox = createMeshBBCube(themesh);

	// Moving mesh to origin
	cout << "Moving mesh to origin ... "; Timer timer = Timer();
	for(size_t i = 0; i < themesh->vertices.size() ; i++){ themesh->vertices[i] = themesh->vertices[i] - mesh_bbox.min;}
	cout << "done in " << timer.getTimeMilliseconds() << " ms." << endl;

	// Write mesh to format we can stream in
	string base = filename.substr(0,filename.find_last_of("."));
	std::string tri_out_name = base+ ".tridata";
	std::string tri_header_out_name = base + ".tri";

	ofstream tri_out;
	tri_out.open(tri_out_name.c_str(), ios::out |ios::binary);

	cout << "Writing mesh triangles ... "; timer.reset();
	Triangle t;
	// Write all triangles to data file
	for(size_t i = 0; i < themesh->faces.size(); i++){
		t.v0 = themesh->vertices[themesh->faces[i][0]];
		t.v1 = themesh->vertices[themesh->faces[i][1]];
		t.v2 = themesh->vertices[themesh->faces[i][2]];
#ifdef BINARY_VOXELIZATION
		writeTriangle(tri_out,t);
#else
		t.normal = computeFaceNormal(themesh,i);
		//t.normal = getShadingFaceNormal(themesh,i);
		writeTriangle(tri_out,t);
#endif
	}
	cout << "done in " << timer.getTimeMilliseconds() << " ms." << endl;

	// Prepare tri_info and write header
	cout << "Writing header ... ";
	TriInfo tri_info;
	tri_info.version = 1;
	tri_info.mesh_bbox = mesh_bbox;
	tri_info.n_triangles = themesh->faces.size();
#ifdef BINARY_VOXELIZATION
	tri_info.geometry_only = 1;
#else
	tri_info.geometry_only = 0;
#endif
	writeTriHeader(tri_header_out_name, tri_info);
	tri_info.print();
}