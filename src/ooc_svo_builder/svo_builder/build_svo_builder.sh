#!/bin/bash

COMPILE="g++ -c -std=c++11 -I../tri_tools/include/ -I ../../../../trimesh2/include/"
COMPILE_BINARY="g++ -c -std=c++11 -I../tri_tools/include/ -I ../../../../trimesh2/include/ -D BINARY_VOXELIZATION"
LINK="g++ -o svo_builder"
LINK_BINARY="g++ -o svo_builder_binary"


## CLEAN
echo "Removing old versions ..."
rm svo_builder
rm svo_builder_binary
rm *.o

## BUILD BINARY VOXELIZATION VERSION
echo "Compiling binary voxelization build..."
${COMPILE_BINARY} main.cpp
${COMPILE_BINARY} OctreeBuilder.cpp
${COMPILE_BINARY} partitioner.cpp
${COMPILE_BINARY} voxelizer.cpp
echo "Linking binary voxelization build..."
${LINK_BINARY} *.o

echo "Cleaning up ..."
rm *.o

## BUILD REGULAR VERSION
echo "Compiling regular version build..."
${COMPILE} main.cpp
${COMPILE} OctreeBuilder.cpp
${COMPILE} partitioner.cpp
${COMPILE} voxelizer.cpp
echo "Linking regular version build..."
${LINK} *.o

echo "Done"
