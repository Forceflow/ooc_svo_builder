#include "OctreeBuilder.h"

// OctreeBuilder constructor: this initializes the builder and sets up the output files, ready to go
OctreeBuilder::OctreeBuilder(std::string base_filename, size_t gridlength, bool generate_levels) :
gridlength(gridlength), b_node_pos(0), b_data_pos(0), b_current_morton(0), generate_levels(generate_levels), base_filename(base_filename) {
	svo_algo_timer.start();

	// Open output files
	string nodes_name = base_filename + string(".octreenodes");
	string data_name = base_filename + string(".octreedata");
	node_out = fopen(nodes_name.c_str(), "wb");
	data_out = fopen(data_name.c_str(), "wb");

	// Setup building variables
	b_maxdepth = log2(static_cast<unsigned int>(gridlength));
	b_buffers.resize(b_maxdepth + 1);
	for (int i = 0; i < b_maxdepth + 1; i++){
		b_buffers[i].reserve(8);
	}

	// Fill data arrays
	uint_fast32_t maxm = static_cast<uint_fast32_t>(gridlength - 1);
	b_max_morton = morton3D_64_encode(maxm,maxm,maxm);
	svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
	writeVoxelData(data_out, VoxelData(), b_data_pos); // first data point is NULL
#ifdef BINARY_VOXELIZATION
	VoxelData v = VoxelData(0, vec3(), vec3(1.0, 1.0, 1.0)); // We store a simple white voxel in case of Binary voxelization
	writeVoxelData(data_out, v, b_data_pos); // all leafs will refer to this
#endif
	svo_io_out_timer.stop(); svo_algo_timer.stop();
}

// Finalize the tree: add rest of empty nodes, make sure root node is on top
void OctreeBuilder::finalizeTree(){
	// fill octree
	if (b_current_morton < b_max_morton){
		fastAddEmpty((b_max_morton - b_current_morton) + 1);
	}

	// write root node
	svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
	writeNode(node_out, b_buffers[0][0], b_node_pos);
	svo_io_out_timer.stop(); svo_algo_timer.start(); // TIMING

	// write header
	OctreeInfo octree_info(1, base_filename, gridlength, b_node_pos, b_data_pos);

	svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
	writeOctreeHeader(base_filename + string(".octree"), octree_info);
	svo_io_out_timer.stop(); svo_algo_timer.start(); // TIMING

	// close files
	fclose(data_out);
	fclose(node_out);
}

// Group 8 nodes, write non-empty nodes to disk and create parent node
Node OctreeBuilder::groupNodes(const vector<Node> &buffer){
	Node parent = Node();
	bool first_stored_child = true;
	for (int k = 0; k < 8; k++){
		if (!buffer[k].isNull()){
			if (first_stored_child){
				svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
				parent.children_base = writeNode(node_out, buffer[k], b_node_pos);
				svo_io_out_timer.stop(); svo_algo_timer.start(); // TIMING
				parent.children_offset[k] = 0;
				first_stored_child = false;
			}
			else {
				svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
				parent.children_offset[k] = (char)(writeNode(node_out, buffer[k], b_node_pos) - parent.children_base);
				svo_io_out_timer.stop(); svo_algo_timer.start(); // TIMING
			}
		}
		else {
			parent.children_offset[k] = NOCHILD;
		}
	}

	// SIMPLE LEVEL CONSTRUCTION
	if (generate_levels){
		VoxelData d = VoxelData();
		float notnull = 0.0f;
		for (int i = 0; i < 8; i++){ // this node has no data: need to refine
			if (!buffer[i].isNull())
				notnull++;
			d.color += buffer[i].data_cache.color;
			d.normal += buffer[i].data_cache.normal;
		}
		d.color = d.color / notnull;
		vec3 tonormalize = (vec3)(d.normal / notnull);
		d.normal = normalize(tonormalize);
		// set it in the parent node
		svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
		parent.data = writeVoxelData(data_out, d, b_data_pos);
		svo_io_out_timer.stop(); svo_algo_timer.start(); // TIMING
		parent.data_cache = d;
	}

	return parent;
}

// Add an empty datapoint at a certain buffer level, and refine upwards from there
void OctreeBuilder::addEmptyVoxel(const int buffer){
	b_buffers[buffer].push_back(Node());
	refineBuffers(buffer);
	b_current_morton = static_cast<::uint64_t>(b_current_morton + pow(8.0, b_maxdepth - buffer)); // because we're adding at a certain level
}

// REFINE BUFFERS: check all levels from start_depth up and group 8 nodes on a higher level
void OctreeBuilder::refineBuffers(const int start_depth){
	for (int d = start_depth; d >= 0; d--){
		if (b_buffers[d].size() == 8){ // if we have 8 nodes
			assert(d - 1 >= 0);
			if (isBufferEmpty(b_buffers[d])){
				b_buffers[d - 1].push_back(Node()); // push back NULL node to represent 8 empty nodes
			}
			else {
				b_buffers[d - 1].push_back(groupNodes(b_buffers[d])); // push back parent node
			}
			b_buffers.at(d).clear(); // clear the 8 nodes on this level
		}
		else {
			break; // break the for loop: no upper levels will need changing
		}
	}
}

// Add a datapoint to the octree: this is the main method used to push datapoints
void OctreeBuilder::addVoxel(const ::uint64_t morton_number){
	// Padding for missed morton numbers
	if (morton_number != b_current_morton){
		fastAddEmpty(morton_number - b_current_morton);
	}

	// Create node
	Node node = Node(); // create empty node
	node.data = 1; // all nodes in binary voxelization refer to this
	// Add to buffer
	b_buffers.at(b_maxdepth).push_back(node);
	// Refine buffers
	refineBuffers(b_maxdepth);

	b_current_morton++;
}

// Add a datapoint to the octree: this is the main method used to push datapoints
void OctreeBuilder::addVoxel(const VoxelData& data){
	// Padding for missed morton numbers
	if (data.morton != b_current_morton){
		fastAddEmpty(data.morton - b_current_morton);
	}

	// Create node
	Node node = Node(); // create empty node
	// Write data point
	svo_algo_timer.stop(); svo_io_out_timer.start(); // TIMING
	node.data = writeVoxelData(data_out, data, b_data_pos); // store data
	svo_io_out_timer.stop(); svo_algo_timer.start(); // TIMING
	node.data_cache = data; // store data as cache
	// Add to buffers
	b_buffers.at(b_maxdepth).push_back(node);
	// Refine buffers
	refineBuffers(b_maxdepth);

	b_current_morton++;
}