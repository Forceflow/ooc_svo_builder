#!/bin/bash

## SPECIFY TRIMESH LOCATION HERE (and do a make there first)
TRIMESH_DIR=/home/jeroen/development/trimesh2
SOURCE_DIR=../src/svo_builder/

## COMPILE AND LINK DEFINITIONS
COMPILE="g++ -g -c -O3 -I../src/libs/tri_tools/include/ -I ${TRIMESH_DIR}/include/"
COMPILE_BINARY="g++ -c -O3 -I../src/libs/tri_tools/include/ -I ${TRIMESH_DIR}/include/ -D BINARY_VOXELIZATION"
LINK="g++ -g -o svo_builder"
LINK_BINARY="g++ -g -o svo_builder_binary"

#############################################################################################
## BUILDING STARTS HERE

## CLEAN
echo "Removing old versions ..."
rm -f svo_builder
rm -f svo_builder_binary
rm -f *.o

## BUILD BINARY VOXELIZATION VERSION
echo "Compiling binary voxelization build..."
${COMPILE_BINARY} ${SOURCE_DIR}main.cpp
${COMPILE_BINARY} ${SOURCE_DIR}OctreeBuilder.cpp
${COMPILE_BINARY} ${SOURCE_DIR}partitioner.cpp
${COMPILE_BINARY} ${SOURCE_DIR}voxelizer.cpp
echo "Linking binary voxelization build..."
${LINK_BINARY} *.o

echo "Cleaning up ..."
rm *.o

## BUILD REGULAR VERSION
echo "Compiling regular version build..."
${COMPILE} ${SOURCE_DIR}main.cpp
${COMPILE} ${SOURCE_DIR}OctreeBuilder.cpp
${COMPILE} ${SOURCE_DIR}partitioner.cpp
${COMPILE} ${SOURCE_DIR}voxelizer.cpp
echo "Linking regular version build..."
${LINK} *.o

echo "Done"
