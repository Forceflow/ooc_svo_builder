# Out-Of-Core SVO Builder v1.2

This is a proof of concept implementation of the algorithm explained in our HPG 2013 paper, Out Of Core Construction of Sparse Voxel Octrees. The paper and additional material can be found on the [project page](http://graphics.cs.kuleuven.be/publications/BLD13OCCSVO/).

There are two tools distributed in this release, both are required to convert a model into a Sparse Voxel Octree representation:

* **tri_convert:** A tool to convert a model file to a simple, streamable .tri format, described in this manual.
* **svo_builder:** Out-Of-Core SVO Builder: Partitioning, voxelizing and SVO Building rolled into one, needs a .tri file as input

## Code / Dependencies
The current **ooc_svo_builder** release consists of:

* Precompiled binaries for Win64 and Linux64
* Source code on Github.
* Visual Studio 2013 project files, Linux build scripts (sh) and Cmake files for OSX

For Linux and OSX, the standard gcc/clang toolchain will do. For Windows64, VS2012 is required - there's a free VS Studio 2012 Express edition [available](http://www.microsoft.com/visualstudio/eng/products/visual-studio-express-products).

The only dependency for compiling from source is the [Trimesh2 library](http://gfx.cs.princeton.edu/proj/trimesh2/) by Szymon Rusinkiewicz, which is used for input/output of triangle meshes and the vector math. It will be statically linked in the binaries.

## Usage / Examples
### Modes: Geometry-only / With-payload voxelization
This release supports two modes of SVO building:
* **Binary-only** SVO building (only geometry, the default mode presented in the paper). The result will be an SVO with all leaf nodes referencing the same, white default voxel.
* **Payload** SVO building with a (normal vector + vertex colors) paylodper voxel. The result will be an SVO with all leaf nodes referencing their own sampled voxel payload.

Throughout all executables, the tools for binary voxelization are postfixed with _binary. During compilation, you can define the preprocessor directive #BINARY_VOXELIZATION to generate the binary-only SVO construction version.

### tri_convert: Converting a model to .tri format
The out-of-core octree builder uses a simple binary format for triangles and their information. Before you can build an SVO from a 3d model, you've got to convert it to the .tri format using the *tri_convert* tool. The bounding box of the model will be padded to be cubical. See Tri file format (further) for more information. Tri_convert accepts .ply, .off, .3ds, .obj, .sm or .ray files. For geometry_only .tri file generation, use *tri_convert_binary*, for .tri file generation with a normal vector payload, use tri_convert.

**Syntax:** tri_convert(_binary) -f (path to model file)

**Example:** 
```
tri_convert(_binary) -f /home/jeroen/bunny.ply
```
This will generate a bunny.tri + bunny.tridata file pair in the same directory

### svo_builder: Out-Of-Core octree building
The out-of-core octree builder takes a .tri file as input and performs the three steps (partitioning, voxelization and SVO building) described in the [paper](http://graphics.cs.kuleuven.be/publications/BLD13OCCSVO/). You can read that for full details, but in short: depending on the memory limit you specify, the model is partitioned into several subgrids in a pre-pass, then each of these subgrids is voxelized and the corresponding part of the SVO is built. The output is stored in the .octree file format, described in this section.

Since v1.2, side-buffer of configurable maximum size is also used to speed up SVO generation. This is especially interesting for sparse models (voxelizations of thin models).

To build an octree for a geometry-only file, use svo_builder_binary. For building an octree for files with a normal vector payload, use svo_builder. The tools will slap you with a trout if you try to run them with the wrong type of file.

**Syntax:** svo_builder(_binary) -options

* **-f** (path to .tri file) : The path to the .tri file you want to build an SVO from. (Required)
* **-s** (gridsize) : The grid size resolution for the SVO. Should be a power of 2. (Default: 1024)
* **-l** (memory limit) : The memory limit for the SVO builder, in Mb. This is where the out-of-core part kicks in, of course. The tool will automatically select the most optimal partition size depending on the given memory limit. (Default: 2048)
* **-d** (percentage sparseness) : How many percent (between 0.00 and 1.00) of the memory limit the process can use extra to speed up SVO generation in the case of Sparse Models. (Default: 0.10)
* **-levels** Generate intermediare SVO levels' voxel payloads by averaging data from lower levels (which is a quick and dirty way to do low-cost Level-Of-Detail hierarchies). If this option is not specified, only the leaf nodes have an actual payload. (Default: off)
* **-c** (color_mode) Generate colors for the voxels. Keep in mind that when you're using the geometry-only version of the tool (svo_builder_binary), all the color options will be ignored and the voxels will just get a fixed white color. Options for color mode: (Default: model) 
 * **model** : Give all voxels the color which is embedded in the .tri file. (Which will be white if the original model contained no vertex color information).
 * **linear** : Give voxels a linear RGB color related to their position in the grid.
 * **normal** : Get colors for voxels from sample normals of original triangles.
 * **fixed** : Give voxels a fixed color, configurable in the source code.
* **-v** Be very verbose, for debugging purposes. Switch this on if you're running into problems.

**Examples**

````
svo_builder_binary -f bunny.tri
````
Will generate a geometry-only SVO file bunny.octree for a 1024^3 grid, using 2048 Mb of system memory.
````
svo_builder -f bunny.tri -s 2048 -l 1024 -c normal -v
````
Will generate a SVO file bunny.octree for a 2048^3 grid, using 1024 Mb of system memory, and be verbose about it. The voxels will have a payload and their colors will be derived from their normal.

## Octree File Format

The .octree file format is a very simple straightforward format which only contains the basic SVO information. It is not optimized for GPU streaming or compact storage, but is easy to parse and convert to whatever you need in your SVO adventures.

It consists of a text-based header (extension .octree) containing the relevant SVO info and two binary data files, with extensions .octreenodes and .octreedata, containing the octree nodes and actual node data payload respectively. This seperation of the SVO from the actual data is done to seperate hot (tree itself) from cold (payload) data, since the payload of course grows quickly when you store more properties in a voxel.

### Octree Header
A typical .octree header file is text-based and looks like this. All keyword - values lines are separated by a newline:
```
#octreeheader 1
gridlength 1024
n_nodes 1474721
n_data 484322
end
```
All elements are required.
* **#octreeheader (version_number)**: (int) Octree version number.
* **gridlength (n)**: (int) Length of one side of the cubical voxel grid. Should be a power of 2.
* **n_nodes (n)**: (int) The total amount of SVO nodes.
* **n_data (n)**: (int) The total amount of data payloads. This is not automatically the same as n_nodes, you can have several nodes point to the same data. In the case of a geometry-only SVO, all nodes refer to the same voxel payload, at position 1.
* **END**: Indicating the end of the header file.

### Octree node file
An .octreenodes file is a very simple binary file which describes the big flat array of octree nodes. In the nodes, there are only child pointers, but it is trivial to extend the code to also store parent/neighbour pointers. The child pointers are constructed from a base address combined with a child offset, since all nonempty children of a certain node are guaranteed by the algorithm to be stored next to eachother. As described in the .octreeheader, the .octreenodes file contains n_nodes nodes.

* **children base address**: (size_t, 64 bits) The base address of the children of this node.
* **child offsets**: (8 * 8 bit char = 64 bits) Children offsets for each of the 8 children. Each is a number between -1 and 7.
 * If the number is >= 0, add it to the children base address the get the actual child address.
 * A child offset of -1 is interpreted as a NULL pointer : there's no child there.
* **data address**: (size_t, 64 bits) Index of data payload in data array described in the .octreedata file (see further).
 * If the address is 0, this is a data NULL pointer : there's no data associated with this node

### Octree data file

An .octreedata file is a binary file representing the big flat array of data payloads. Nodes in the octree refer to their data payload by using a 64-bit pointer, which corresponds to the index in this data array. The first data payload in this array is always the one representing an empty payload. Nodes refer to this if they have no payload (internal nodes in the tree, ...). 

The current payload contains a morton code, normal and color information. This can be easily extended with more appearance data. I refer to the 'Appearance' section in our paper. As described in the .octreeheader, the .octreedata file contains n_data nodes.

* **morton:** (64 bit unsigned int) Morton code of this voxel payload
* **color:** (3 * 32 bit float = 96 bits) RGB color, three float values between 0 and 1.
* **normal vector:** (3 * 32 bit float = 96 bits) x, y and z components of normal vector of this voxel payload.

## Tri File Format
The .tri file format is a very simple internal format to make abstraction of all the pitfalls and peculiarities of the plethora of model input formats in the wild. It basicly lists all triangles as one long list, with optional triangle properties (normals, colors ...) right behind the triangle definition. It was created with regards to streamability: all triangle information is stored sequentially, so you don't need to load the whole file in memory to process triangles. It's not as space-efficient as other formats, but is very easy to understand and parse.

The .tri file format consists of a text-based header (extension .tri) containing the relevant model info and a binary data file (extension .tridata) containing the actual model data. 

###Tri Header
A typical .tri header file is text-based and looks like this. All keyword - values lines are separated by a newline:
```
#tri 1
ntriangles 8254150
geo_only 1
bbox  -2540.83 -2586 -15957 2699.43 2654.25 -10716.8
END
```
All elements are required.
* **#tri (version_number)**: (int) Tri version number.
* **ntriangles (n)**: (size_t) Total number of triangles.
* **geo_only (0 or 1)**: (bool) Indicate whether or not the data is geometry-only (for binary voxelization).
* **bbox (min_x min_y min_z max_x max_y max_z)**: (floats) Minimum and maximum vector of bounding box of the mesh. Bounding box must be cubical, or your model will be stretched during voxelization.
* **END**: Indicating the end of the header file.

### Tri data file
A .tridata file is a very simple binary file which just contains a list of the x, y and z coordinates of the triangle vertices (v0, v1 and v2), followed by the optional payload per triangle (normal, texture information, ...).

In case of geometry-only (or, confusingly called binary, as in binary voxelization) .tridata files, the layout per triangle looks like this:

* vertex 0 (3 x 32 bit float)
* vertex 1 (3 x 32 bit float)
* vertex 2 (3 x 32 bit float)

In case of a payload .tridata file, the layout per triangle is followed by

* normal (3 x 32 bit float)
* vertex 0 color (3 x 32 bit float)
* vertex 1 color (3 x 32 bit float)
* vertex 2 color (3 x 32 bit float)

## Acknowledgements
I would like to thank Ares Lagae and Philip Dutre for their continuing great input and co-authorship. We would also like to thank the anonymous reviewers for their remarks and comments. Jeroen Baert is funded by the Agency for Innovation by Science and Technology in Flanders (IWT). Ares Lagae is a postdoctoral fellow of the Research Foundation - Flanders (FWO).

## Contact
Feel free to contact me with questions/suggestions at jeroen.baert (at) cs.kuleuven.be

Keep in mind that this tool is in active development and that it is research code. I am not responsible or liable if it crashes your system, runs over your dog and/or steals your partner.