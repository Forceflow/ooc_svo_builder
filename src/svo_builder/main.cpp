#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
#define WINDOWS_LEAN_AND_MEAN
#endif

// If you have a CPU which supports BMI2 / AVX2, compile using this flag to speed up Morton encoding/decoding
// #define __BMI2__

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <sstream>
#include "globals.h"
#include "../libs/libtri/include/trip_tools.h"
#include "../libs/libtri/include/TriReader.h"

#include "voxelizer.h"
#include "OctreeBuilder.h"
#include "partitioner.h"

using namespace std;
using namespace glm;

enum ColorType { COLOR_FROM_MODEL, COLOR_FIXED, COLOR_LINEAR, COLOR_NORMAL };

// Program version
string version = "1.6 alpha";

// Program parameters
string filename = "";
size_t gridsize = 1024;
size_t voxel_memory_limit = 2048;
float sparseness_limit = 0.10f;
ColorType color = COLOR_FROM_MODEL;
vec3 fixed_color = vec3(1.0f, 1.0f, 1.0f); // fixed color is white
bool generate_levels = false;
bool verbose = false;

// trip header info
TriInfo tri_info;
TripInfo trip_info;

// buffer_size
size_t input_buffersize = 8192;

// timers
Timer main_timer;
Timer part_total_timer;
Timer part_io_in_timer;
Timer part_io_out_timer;
Timer part_algo_timer;
Timer vox_total_timer;
Timer vox_io_in_timer;
Timer vox_algo_timer;
Timer svo_total_timer;
Timer svo_io_out_timer;
Timer svo_algo_timer;

void printInfo() {
	cout << "--------------------------------------------------------------------" << endl;
#ifdef BINARY_VOXELIZATION
	cout << "Out-Of-Core SVO Builder " << version << " - Geometry only version" << endl;
#else
	cout << "Out-Of-Core SVO Builder " << version << endl;
#endif
#if defined(_WIN32) || defined(_WIN64)
	cout << "Windows " << endl;
#endif
#ifdef __linux__
	cout << "Linux " << endl;
#endif
#ifdef _WIN64
	cout << "64-bit version" << endl;
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
	std::cout << "-c <option>           Coloring of voxels (Options: model (default), fixed, linear, normal)" << endl;
	std::cout << "-d <percentage>       Percentage of memory limit to be used additionaly for sparseness optimization" << endl;
	std::cout << "-v                    Be very verbose." << endl;
	std::cout << "-h                    Print help and exit." << endl;
}

void printInvalid() {
	std::cout << "Not enough or invalid arguments, please try again." << endl;
	std::cout << "At the bare minimum, I need a path to a .TRI file" << endl << "" << endl;
	printHelp();
}

// Parse command-line params and so some basic error checking on them
void parseProgramParameters(int argc, char* argv[]) {
	string color_s = "Color from model (fallback to fixed color if model has no color)";
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
		}
		else if (string(argv[i]) == "-s") {
			gridsize = atoi(argv[i + 1]);
			if (!isPowerOf2((unsigned int) gridsize)) {
				cout << "Requested gridsize is not a power of 2" << endl;
				printInvalid();
				exit(0);
			}
			i++;
		}
		else if (string(argv[i]) == "-l") {
			voxel_memory_limit = atoi(argv[i + 1]);
			if (voxel_memory_limit <= 1) {
				cout << "Requested memory limit is nonsensical. Use a value >= 1" << endl;
				printInvalid();
				exit(0);
			}
			i++;
		}
		else if (string(argv[i]) == "-d") {
			int percent = atoi(argv[i + 1]);
			sparseness_limit = percent / 100.0f;
			if (sparseness_limit < 0) {
				cout << "Requested data memory limit is nonsensical. Use a value > 0" << endl;
				printInvalid();
				exit(0);
			}
			i++;
		}
		else if (string(argv[i]) == "-v") {
			verbose = true;
		}
		else if (string(argv[i]) == "-levels") {
			generate_levels = true;
		}
		else if (string(argv[i]) == "-c") {
			string color_input = string(argv[i + 1]);
#ifdef BINARY_VOXELIZATION
			cout << "You asked to generate colors, but we're only doing binary voxelisation." << endl;
#else
			if (color_input == "model") {
				color = COLOR_FROM_MODEL;
			}
			else if (color_input == "linear") {
				color = COLOR_LINEAR;
				color_s = "Linear";
			}
			else if (color_input == "normal") {
				color = COLOR_NORMAL;
				color_s = "Normal";
			}
			else if (color_input == "fixed") {
				color = COLOR_FIXED;
				color_s = "Fixed";
			}
			else {
				cout << "Unrecognized color switch: " << color_input << ", so reverting to colors from model." << endl;
			}
#endif
			i++;
		}
		else if (string(argv[i]) == "-h") {
			printHelp(); exit(0);
		}
		else {
			printInvalid(); exit(0);
		}
	}
	if (verbose) {
		cout << "  filename: " << filename << endl;
		cout << "  gridsize: " << gridsize << endl;
		cout << "  memory limit: " << voxel_memory_limit << endl;
		cout << "  sparseness optimization limit: " << sparseness_limit << " resulting in " << (sparseness_limit*voxel_memory_limit) << " memory limit." << endl;
		cout << "  color type: " << color_s << endl;
		cout << "  generate levels: " << generate_levels << endl;
		cout << "  verbosity: " << verbose << endl;
	}
}

// Initialize all performance timers
void setupTimers() {
	main_timer = Timer();

	// Partitioning timers
	part_total_timer = Timer();
	part_io_in_timer = Timer();
	part_io_out_timer = Timer();
	part_algo_timer = Timer();

	// Voxelization timers
	vox_total_timer = Timer();
	vox_io_in_timer = Timer();
	vox_algo_timer = Timer();

	svo_total_timer = Timer();
	svo_io_out_timer = Timer();
	svo_algo_timer = Timer();
}

// Printout total time of running Timers (for debugging purposes)
void printTimerInfo() {
	//double diff = main_timer.elapsed_time_milliseconds - (algo_timer.elapsed_time_milliseconds + io_timer_in.elapsed_time_milliseconds + io_timer_out.elapsed_time_milliseconds);
	cout << "Total MAIN time      : " << main_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "PARTITIONING" << endl;
	cout << "  Total time		: " << part_total_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  IO IN time		: " << part_io_in_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  algorithm time	: " << part_algo_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  IO OUT time		: " << part_io_out_timer.elapsed_time_milliseconds << " ms." << endl;
	double part_diff = part_total_timer.elapsed_time_milliseconds - part_io_in_timer.elapsed_time_milliseconds - part_algo_timer.elapsed_time_milliseconds - part_io_out_timer.elapsed_time_milliseconds;
	cout << "  misc time		: " << part_diff << " s." << endl;
	cout << "VOXELIZING" << endl;
	cout << "  Total time		: " << vox_total_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  IO IN time		: " << vox_io_in_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  algorithm time	: " << vox_algo_timer.elapsed_time_milliseconds << " ms." << endl;
	double vox_diff = vox_total_timer.elapsed_time_milliseconds - vox_io_in_timer.elapsed_time_milliseconds - vox_algo_timer.elapsed_time_milliseconds;
	cout << "  misc time		: " << vox_diff << " s." << endl;
	cout << "SVO BUILDING" << endl;
	cout << "  Total time		: " << svo_total_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  IO OUT time		: " << svo_io_out_timer.elapsed_time_milliseconds << " ms." << endl;
	cout << "  algorithm time	: " << svo_algo_timer.elapsed_time_milliseconds << " ms." << endl;
	double svo_misc = svo_total_timer.elapsed_time_milliseconds - svo_io_out_timer.elapsed_time_milliseconds - svo_algo_timer.elapsed_time_milliseconds;
	cout << "  misc time		: " << svo_misc << " s." << endl;
}

// Tri header handling and error checking
void readTriHeader(string& filename, TriInfo& tri_info){
	cout << "Parsing tri header " << filename << " ..." << endl;
	if (parseTriHeader(filename, tri_info) != 1) {
		exit(0); // something went wrong in parsing the header - exiting.
	}
	// disabled for benchmarking
	if (!tri_info.filesExist()) {
		cout << "Not all required .tri or .tridata files exist. Please regenerate using tri_convert." << endl; 
		exit(0); // not all required files exist - exiting.
	}
	if (verbose) { tri_info.print(); }
	// Check if the user is using the correct executable for type of tri file
#ifdef BINARY_VOXELIZATION
	if (!tri_info.geometry_only) {
		cout << "You're using a .tri file which contains more than just geometry with a geometry-only SVO Builder! Regenerate that .tri file using tri_convert_binary." << endl;
		exit(0);
	}
#else
	if (tri_info.geometry_only) {
		cout << "You're using a .tri file which contains only geometry with the regular SVO Builder! Regenerate that .tri file using tri_convert." << endl;
		exit(0);
	}
#endif
}

// Trip header handling and error checking
void readTripHeader(string& filename, TripInfo& trip_info){
	if (parseTripHeader(filename, trip_info) != 1) {
		exit(0);
	}
	if (!trip_info.filesExist()) {
		cout << "Not all required .trip or .tripdata files exist. Please regenerate using svo_builder." << endl; 
		exit(0); // not all required files exist - exiting.
	}
	if (verbose) { trip_info.print(); }
}

int main(int argc, char *argv[]) {
	// Setup timers
	setupTimers();
	main_timer.start();

#if defined(_WIN32) || defined(_WIN64)
	_setmaxstdio(1024); // increase file descriptor limit in Windows
#endif

	// Parse program parameters
	printInfo();
	parseProgramParameters(argc, argv);

	// PARTITIONING
	part_total_timer.start(); part_io_in_timer.start(); // TIMING
	readTriHeader(filename, tri_info);
	part_io_in_timer.stop();
	size_t n_partitions = estimate_partitions(gridsize, voxel_memory_limit);
	cout << "Partitioning data into " << n_partitions << " partitions ... "; cout.flush();
	TripInfo trip_info = partition(tri_info, n_partitions, gridsize);
	cout << "done." << endl;
	part_total_timer.stop(); // TIMING

	vox_total_timer.start(); vox_io_in_timer.start(); // TIMING
	// Parse TRIP header
	string tripheader = trip_info.base_filename + string(".trip");
	readTripHeader(tripheader, trip_info);
	vox_io_in_timer.stop(); // TIMING

	// General voxelization calculations (stuff we need throughout voxelization process)
	float unitlength = (trip_info.mesh_bbox.max[0] - trip_info.mesh_bbox.min[0]) / (float)trip_info.gridsize;
	::uint64_t morton_part = (trip_info.gridsize * trip_info.gridsize * trip_info.gridsize) / trip_info.n_partitions;

	char* voxels = new char[(size_t)morton_part]; // Storage for voxel on/off
#ifdef BINARY_VOXELIZATION
	vector<::uint64_t> data; // Dynamic storage for morton codes
#else
	vector<VoxelData> data; // Dynamic storage for voxel data
#endif 
	size_t nfilled = 0;
	vox_total_timer.stop(); // TIMING

	svo_total_timer.start();
	// create Octreebuilder which will output our SVO
	OctreeBuilder builder = OctreeBuilder(trip_info.base_filename, trip_info.gridsize, generate_levels);
	svo_total_timer.stop();

	// Start voxelisation and SVO building per partition
	for (size_t i = 0; i < trip_info.n_partitions; i++) {
		if (trip_info.part_tricounts[i] == 0) { continue; } // skip partition if it contains no triangles

		// VOXELIZATION
		vox_total_timer.start(); // TIMING
		cout << "Voxelizing partition " << i << " ..." << endl;
		// morton codes for this partition
		::uint64_t start = i * morton_part;
		::uint64_t end = (i + 1) * morton_part;
		// open file to read triangles
		vox_io_in_timer.start(); // TIMING
		std::string part_data_filename = trip_info.base_filename + string("_") + val_to_string(i) + string(".tripdata");
		TriReader reader = TriReader(part_data_filename, trip_info.part_tricounts[i], std::min(trip_info.part_tricounts[i], input_buffersize));
		if (verbose) { cout << "  reading " << trip_info.part_tricounts[i] << " triangles from " << part_data_filename << endl; }
		vox_io_in_timer.stop(); // TIMING
		// voxelize partition
		size_t nfilled_before = nfilled;
		bool use_data = true;
		voxelize_schwarz_method(reader, start, end, unitlength, voxels, data, sparseness_limit, use_data, nfilled);
		if (verbose) { cout << "  found " << nfilled - nfilled_before << " new voxels." << endl; }
		vox_total_timer.stop(); // TIMING

		// build SVO
		cout << "Building SVO for partition " << i << " ..." << endl;
		svo_total_timer.start(); svo_algo_timer.start(); // TIMING
#ifdef BINARY_VOXELIZATION
		if (use_data){ // use array of morton codes to build the SVO
			sort(data.begin(), data.end()); // sort morton codes
			for (std::vector<::uint64_t>::iterator it = data.begin(); it != data.end(); ++it){
				builder.addVoxel(*it);
			}
		}
		else { // morton array overflowed : using slower way to build SVO
			::uint64_t morton_number;
			for (size_t j = 0; j < morton_part; j++) {
				if (!voxels[j] == EMPTY_VOXEL) {
					morton_number = start + j;
					builder.addVoxel(morton_number);
				}
			}
		}
#else
		sort(data.begin(), data.end()); // sort
		for (std::vector<VoxelData>::iterator it = data.begin(); it != data.end(); ++it){
			if (color == COLOR_FIXED){
				it->color = fixed_color;
			}
			else if (color == COLOR_LINEAR){ // linear color scale
				it->color = mortonToRGB(it->morton, gridsize);
			}
			else if (color == COLOR_NORMAL){ // color models using their normals
				vec3 normal = normalize(it->normal);
				it->color = vec3((normal[0] + 1.0f) / 2.0f, (normal[1] + 1.0f) / 2.0f, (normal[2] + 1.0f) / 2.0f);
			}
			builder.addVoxel(*it);
		}
#endif
		svo_algo_timer.stop(); svo_total_timer.stop();  // TIMING
	}
	svo_total_timer.start(); svo_algo_timer.start(); // TIMING
	builder.finalizeTree(); // finalize SVO so it gets written to disk
	cout << "done" << endl;
	cout << "Total amount of voxels: " << nfilled << endl;
	svo_total_timer.stop(); svo_algo_timer.stop(); // TIMING

	// Removing .trip files which are left by partitioner
	removeTripFiles(trip_info);

	main_timer.stop();
	printTimerInfo();
}