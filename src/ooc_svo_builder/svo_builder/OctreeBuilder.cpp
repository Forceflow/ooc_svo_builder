#include "OctreeBuilder.h"

// OctreeBuilder constructor: this initializes the builder and sets up the output files, ready to go
OctreeBuilder::OctreeBuilder(std::string basefilename, size_t gridlength, bool fast_empty, bool generate_levels) :
gridlength(gridlength), b_node_pos(0), b_data_pos(0), b_current_morton(0), fast_empty(fast_empty), generate_levels(generate_levels){
	// Open output files
	octreeheader_name = basefilename + string(".octree");
	string nodes_name = basefilename + string(".octreenodes");
	string data_name = basefilename + string(".octreedata");
	node_out = fopen(nodes_name.c_str(), "wb");
	data_out = fopen(data_name.c_str(), "wb");
	// Setup building variables
	b_maxdepth = log2((unsigned int) gridlength);
	b_buffers.resize(b_maxdepth+1);
	for(int i = 0; i < b_maxdepth+1; i++){
		b_buffers[i].reserve(8);
	}
	b_max_morton = mortonEncode(gridlength-1,gridlength-1,gridlength-1);
	writeDataPoint(data_out, DataPoint(), b_data_pos); // first data point is NULL
#ifdef BINARY_VOXELIZATION
	writeDataPoint(data_out, DataPoint(), b_data_pos); // second data point is NULL, all voxels refer to this if binary voxelization only
#endif
}

// Finalize the tree: add rest of empty nodes, make sure root node is on top
void OctreeBuilder::finalizeTree(){
	// fill octree
	if(b_current_morton < b_max_morton){
		if(fast_empty){
			fastAddEmpty((b_max_morton - b_current_morton)+1);
		} else {
			for(uint64_t i = b_current_morton; i <= b_max_morton; i++){
				addDataPoint(i, DataPoint());
			}
		}
	}
	// write root node
	writeNode(node_out, b_buffers[0][0], b_node_pos);

	// write header
	OctreeInfo i;
	i.gridlength = gridlength;
	i.n_data = b_data_pos;
	i.n_nodes = b_node_pos;
	i.version = 1;

	algo_timer.stop(); io_timer_out.start(); // TIMING
	writeOctreeHeader(octreeheader_name,i);
	algo_timer.stop(); io_timer_out.start(); // TIMING

	// close files
	fclose(data_out);
	fclose(node_out);
}

// Group 8 nodes, write non-empty nodes to disk and create parent node
Node OctreeBuilder::groupNodes(const vector<Node> &buffer){
	Node parent = Node();
	bool first_stored_child = true;
	for(int k = 0; k<8; k++){
		if(!buffer[k].isNull()){
			if(first_stored_child){
					parent.children_base = writeNode(node_out,buffer[k],b_node_pos);
					parent.children_offset[k] = 0;
					first_stored_child = false;
			} else {
				parent.children_offset[k] = writeNode(node_out,buffer[k],b_node_pos) - parent.children_base;
			}
		} else {
			parent.children_offset[k] = NOCHILD;
		}
	}

	// SIMPLE LEVEL CONSTRUCTION
	if(generate_levels){
		DataPoint d = DataPoint();
		float notnull = 0.0f;
		for(int i = 0; i < 8; i++){ // this node has no data: need to refine
			if(!buffer[i].isNull())
				notnull++;
				d.opacity += buffer[i].data_cache.opacity;
				d.color += buffer[i].data_cache.color;
				d.normal += buffer[i].data_cache.normal;
		}
		d.color = d.color / notnull;
		vec3 tonormalize = (vec3) (d.normal / notnull);
		d.normal = normalize(tonormalize);
		d.opacity = d.opacity / notnull;
		// set it in the parent node
		parent.data = writeDataPoint(data_out, d, b_data_pos);
		parent.data_cache = d;
	}
	return parent;
}

// Add an empty datapoint at a certain buffer level, and refine upwards from there
void OctreeBuilder::addEmptyDataPoint(int buffer){
	b_buffers[buffer].push_back(Node());
	// REFINE BUFFERS: check from touched buffer, upwards
	for(int d = buffer; d >= 0; d--){
		if(b_buffers[d].size() == 8){ // if we have 8 nodes
			assert(d-1 >= 0);
			if(isBufferEmpty(b_buffers[d])){
				b_buffers[d-1].push_back(Node()); // push back NULL node to represent 8 empty nodes
			} else { 
				b_buffers[d-1].push_back(groupNodes(b_buffers[d])); // push back parent node
			}
			b_buffers.at(d).clear(); // clear the 8 nodes on this level
		} else {
			break; // break the for loop: no upper levels will need changing
		}
	}
	b_current_morton = b_current_morton + pow(8.0,b_maxdepth-buffer); // because we're adding at a certain level
}

// Add a datapoint to the octree: this is the main method used to push datapoints<
void OctreeBuilder::addDataPoint(uint64_t morton_number, DataPoint point){
	// PADDING FOR MISSED MORTON NUMBERS
	if(morton_number != b_current_morton){
		if(fast_empty){
			fastAddEmpty(morton_number - b_current_morton);
		} else {
			for(uint64_t i = b_current_morton; i < morton_number; i++){
				addDataPoint(i, DataPoint());
			}
		}
	}

	// ADD NODE TO BUFFER
	Node node = Node(); // create empty node
	if(!point.isEmpty()) {
#ifdef BINARY_VOXELIZATION
		node.data = 1; 
#else
		node.data = writeDataPoint(data_out, point, b_data_pos); // store data
#endif
		node.data_cache = point; // store data as cache
	} 
	b_buffers.at(b_maxdepth).push_back(node);

	// REFINE BUFFERS: check all levels (bottom up) and group 8 nodes on a higher level
	for(int d = b_maxdepth; d >= 0; d--){
		if(b_buffers[d].size() == 8){ // if we have 8 nodes
			assert(d-1 >= 0);
			if(isBufferEmpty(b_buffers[d])){
				b_buffers[d-1].push_back(Node()); // push back NULL node to represent 8 empty nodes
			} else { 
				b_buffers[d-1].push_back(groupNodes(b_buffers[d])); // push back parent node
			}
			b_buffers.at(d).clear(); // clear the 8 nodes on this level
		} else {
			break; // break the for loop: no upper levels will need changing
		}
	}

	// INCREASE CURRENT MORTON NUMBER
	b_current_morton++;
}