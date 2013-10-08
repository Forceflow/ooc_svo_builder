#include <TriMesh.h>
#include <vector>
#include <string>
#include <sstream>
#include <trip_tools.h>
#include <TriReader.h>
#include "globals.h"
#include "voxelizer.h"
#include "OctreeBuilder.h"
#include "partitioner.h"
//#include <omp.h>

using namespace std;

#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

enum ColorType {COLOR_FROM_MODEL, COLOR_FIXED, COLOR_LINEAR, COLOR_NORMAL};

// Program version
string version = "1.1";

// Program parameters
string filename = "";
size_t gridsize = 1024;
size_t memory_limit = 2048;
ColorType color = COLOR_FROM_MODEL;
vec3 fixed_color = vec3(1.0f,1.0f,1.0f); // fixed color is white
bool generate_levels = false;
bool verbose = false;

// trip header info
TriInfo tri_info;
TripInfo trip_info;

// buffer_size
size_t input_buffersize = 500000;

// timers
Timer main_timer, algo_timer, io_timer_in, io_timer_out;

void printInfo() {
	cout << "--------------------------------------------------------------------" << endl;
#ifdef BINARY_VOXELIZATION
	cout << "Out-Of-Core SVO Builder " << version << " - Geometry only version" << endl;
#else
	cout << "Out-Of-Core SVO Builder " << version << " - Geometry+normals version" << endl;
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
	cout << "--------------------------------------------------------------------" << endl << endl;
}

void printHelp() {
	std::cout << "Example: svo_builder -f /home/jeroen/bunny.tri" << endl;
	std::cout << "" << endl;
	std::cout << "All available program options:" << endl;
	std::cout << "" << endl;
	std::cout << "-f <filename.tri>     Path to a .tri input file." << endl;
	std::cout << "-s <gridsize>         Voxel gridsize, should be a power of 2. Default 512." << endl;
	std::cout << "-l <memory_limit>     Memory limit for process, in Mb. Default 1024." << endl;
	std::cout << "-levels               Generate intermediary voxel levels by averaging voxel data" << endl;
	std::cout << "-c <option>           Coloring of voxels (Options: from_model (default), fixed, linear, normal)" << endl;
	std::cout << "-v                    Be very verbose." << endl;
	std::cout << "-h                    Print help and exit." << endl;
}

void printInvalid() {
	std::cout << "Not enough or invalid arguments, please try again." << endl;
	std::cout << "At the bare minimum, I need a path to a .TRI file" << endl<< "" << endl;
	printHelp();
}

// Parse command-line params and so some basic error checking on them
void parseProgramParameters(int argc, char* argv[]) {
	cout << "Reading program parameters ..." << endl;
	// Input argument validation
	if (argc < 3) {
		printInvalid();
		exit(0);
	}
	for (int i = 1; i < argc; i++) {
		// parse filename
		if (string(argv[i]) == "-f") {
			filename = argv[i + 1];
			size_t check_tri = filename.find(".tri");
			if (check_tri == string::npos) {
				cout << "Data filename does not end in .tri - I only support that file format" << endl;
				printInvalid();
				exit(0);
			}
			i++;
		} else if (string(argv[i]) == "-s") {
			gridsize = atoi(argv[i + 1]);
			if (!isPowerOf2(gridsize)) {
				cout << "Requested gridsize is not a power of 2" << endl;
				printInvalid();
				exit(0);
			}
			i++;
		} else if (string(argv[i]) == "-l") {
			memory_limit = atoi(argv[i + 1]);
			if (memory_limit <= 1) {
				cout << "Requested memory limit is nonsensical. Use a value 1>= 0" << endl;
				printInvalid();
				exit(0);
			}
			i++;
		} else if (string(argv[i]) == "-v") {
			verbose = true;
		} else if (string(argv[i]) == "-levels") {
			generate_levels = true;
		} else if (string(argv[i]) == "-c") {
			string color_input = string(argv[i+1]);
#ifdef BINARY_VOXELIZATION
			cout << "You asked to generate colors, but we're only doing binary voxelisation." << endl;
#else
			if(color_input == "linear") {
				color = COLOR_LINEAR;
			} else if(color_input == "normal") { 
				color = COLOR_NORMAL;
			} else {
				cout << "Unrecognized color switch: " << color_input << ", so reverting to no colors." << endl;
			}
#endif
			i++;
		} else if (string(argv[i]) == "-h") {
			printHelp(); exit(0);
		} else {
			printInvalid(); exit(0);
		}
	}
	if (verbose) {
		cout << "  filename: " << filename << endl;
		cout << "  gridsize: " << gridsize << endl;
		cout << "  memory limit: " << memory_limit << endl;
		cout << "  color type: " << color << endl;
		cout << "  generate levels: " << generate_levels << endl;
		cout << "  verbosity: " << verbose << endl;
	}
}

// Initialize all performance timers
void setupTimers() {
	main_timer = Timer();
	algo_timer = Timer();
	io_timer_in = Timer();
	io_timer_out = Timer();
}

// Printout total time of running Timers (for debugging purposes)
void printTimerInfo() {
	double diff = main_timer.getTotalTimeSeconds() - (algo_timer.getTotalTimeSeconds() + io_timer_in.getTotalTimeSeconds() + io_timer_out.getTotalTimeSeconds());
	cout << "Total MAIN time      : " << main_timer.getTotalTimeSeconds() << " s." << endl;
	cout << "Total IO IN time     : " << io_timer_in.getTotalTimeSeconds() << " s." << endl;
	cout << "Total algorithm time : " << algo_timer.getTotalTimeSeconds() << " s." << endl;
	cout << "Total IO OUT time    : " << io_timer_out.getTotalTimeSeconds() << " s." << endl;
	cout << "Total misc time      : " << diff << " s." << endl;
}

// Tri header handling and error checking
void readTriHeader(string& filename, TriInfo& tri_info){
	io_timer_in.start();
	cout << "Parsing tri header " << filename << " ..." << endl;
	if (parseTriHeader(filename, tri_info) != 1) {
		exit(0); // something went wrong in parsing the header - exiting.
	}
	if (!tri_info.filesExist()) {
		cout << "Not all required .tri or .tridata files exist. Please regenerate using tri_convert." << endl; 
		exit(0); // not all required files exist - exiting.
	}
	if (verbose) {tri_info.print();}
	io_timer_in.stop();
	// Check if the user is using the correct executable for type of tri file
#ifdef BINARY_VOXELIZATION
	if (!tri_info.geometry_only) {
		cout << "You're using a .tri file which contains more than just geometry with a geometry-only SVO Builder! Regenerate that .tri file using tri_convert_binary." << endl;
		exit(0);
	}
#else
	if (tri_info.geometry_only) {
		cout << "You're using a .tri file which contains only geometry with the geometry+normals SVO Builder! Regenerate that .tri file using tri_convert." << endl;
		exit(0);
	}
#endif
}

// Trip header handling and error checking
void readTripHeader(string& filename, TripInfo& trip_info){
	io_timer_in.start(); // TIMING
	if (parseTripHeader(filename, trip_info) != 1) {
		exit(0);
	}
	if (!trip_info.filesExist()) {
		cout << "Not all required .trip or .tripdata files exist. Please regenerate using svo_builder." << endl; 
		exit(0); // not all required files exist - exiting.
	}
	if (verbose) {trip_info.print();}
	io_timer_in.stop(); // TIMING
}

int main(int argc, char *argv[]) {
	// Setup timers
	setupTimers();
	main_timer.start();

#ifdef _WIN32 || _WIN64
	_setmaxstdio(1024); // increase file descriptor limit in Windows
#endif

	//// Get the number of processors in this system
	//int iCPU = omp_get_num_procs();
	//omp_set_num_threads(iCPU);

	// Parse program parameters
	printInfo();
	parseProgramParameters(argc, argv);

	// Parse TRI header
	readTriHeader(filename, tri_info);

	// Do partitioning and store results/file refs in TripInfo
	size_t n_partitions = estimate_partitions(gridsize, memory_limit);
	cout << "Partitioning data into " << n_partitions << " partitions ... "; cout.flush();
	algo_timer.start(); // TIMING
	TripInfo trip_info = partition(tri_info, n_partitions, gridsize);
	algo_timer.stop(); // TIMING
	cout << "done." << endl;

	// Parse TRIP header
	readTripHeader(trip_info.base_filename + string(".trip"), trip_info);

	// General voxelization calculations (stuff we need throughout voxelization process)
	float unitlength = (trip_info.mesh_bbox.max[0] - trip_info.mesh_bbox.min[0]) / (float) trip_info.gridsize;
	uint64_t morton_part = (trip_info.gridsize * trip_info.gridsize * trip_info.gridsize) / trip_info.n_partitions;
	
	// Storage for voxel references (STATIC)
#ifdef BINARY_VOXELIZATION
	bool* voxels = new bool[(size_t) morton_part]; // TODO: If you want tighter packing, check the Frankensteiny that is vector<bool>
#else
	size_t* voxels = new size_t[(size_t) morton_part];
	vector<VoxelData> voxel_data; // Storage for voxel data (DYNAMIC)
#endif 

	size_t nfilled = 0;

	// create Octreebuilder which will output our SVO
	OctreeBuilder builder = OctreeBuilder(trip_info.base_filename, trip_info.gridsize, true, generate_levels);

	// Start voxelisation and SVO building per partition
	algo_timer.start(); // TIMING
	for (size_t i = 0; i < trip_info.n_partitions; i++) {
		if (trip_info.part_tricounts[i] > 0) { // if this partition contains triangles
			cout << "Voxelizing partition " << i << " ..." << endl;
			// morton codes for this partition
			uint64_t start = i * morton_part;
			uint64_t end = (i + 1) * morton_part;

			// open file to read triangles
			std::string part_data_filename = trip_info.base_filename + string("_") + val_to_string(i) + string(".tripdata");
			TriReader reader = TriReader(part_data_filename, trip_info.part_tricounts[i], min(trip_info.part_tricounts[i], input_buffersize));
			if (verbose) {cout << "  reading " << trip_info.part_tricounts[i] << " triangles from " << part_data_filename << endl;}

			// voxelize partition
			size_t nfilled_before = nfilled;
#ifdef BINARY_VOXELIZATION
			voxelize_partition(reader, start, end, unitlength, voxels, nfilled);
#else
			voxelize_partition(reader, start, end, unitlength, voxels, voxel_data, nfilled);
#endif
			if (verbose) {cout << "  found " << nfilled - nfilled_before << " new voxels." << endl;}

			// build SVO
			cout << "Building SVO for partition " << i << " ..." << endl;
			uint64_t morton_number;
			DataPoint d;
			for (size_t j = 0; j < morton_part; j++) {
				if (! voxels[j] == EMPTY_VOXEL) {
					morton_number = start + j;
					d = DataPoint();
					d.opacity = 1.0; // this voxel is filled
#ifndef BINARY_VOXELIZATION
					VoxelData& current_data = voxel_data.at(voxels[j]);
					// NORMALS
					d.normal = current_data.normal;
					// COLORS
					if (color = COLOR_FROM_MODEL){
						d.color = current_data.color;
					} else if (color == COLOR_FIXED){
						d.color = fixed_color;
					} else if (color == COLOR_LINEAR){ // linear color scale
						d.color = mortonToRGB(morton_number, gridsize);
					} else if (color == COLOR_NORMAL){ // color models using their normals
						vec3 normal = normalize(d.normal);
						d.color = vec3((normal[0]+1.0f)/2.0f, (normal[1]+1.0f)/2.0f, (normal[2]+1.0f)/2.0f);
					}
#endif
					builder.addDataPoint(morton_number, d); // add data point to SVO building algorithm
				}
			}
		}
	}
	builder.finalizeTree(); // finalize SVO so it gets written to disk
	algo_timer.stop(); // TIMING
	cout << "done" << endl;

	// Removing .trip files which are left by partitioner
	io_timer_out.start(); // TIMING
	removeTripFiles(trip_info);
	io_timer_out.stop(); // TIMING

	main_timer.stop();
	if (verbose) {printTimerInfo();}
}
