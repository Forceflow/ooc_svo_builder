## SPECIFY TRIMESH LOCATION HERE (and do a make there first)
TRIMESH_DIR=/home/jeroen/development/trimesh2

## COMPILE AND LINK DEFINITIONS
COMPILE="g++ -c -O3 -I../tri_tools/include/ -I ${TRIMESH_DIR}/include/"
COMPILE_BINARY="g++ -O3 -c -I../tri_tools/include/ -I ${TRIMESH_DIR}/include/ -D BINARY_VOXELIZATION"
LINK="g++ -o tri_convert"
LINK_BINARY="g++ -o tri_convert_binary"
LINK_OPTS="-L${TRIMESH_DIR}/lib.Linux64 -ltrimesh -fopenmp -static"

#############################################################################################

## CLEAN
echo "Cleaning old versions ..."
rm *.o
rm tri_convert
rm a.out

## BUILD BINARY VOXELIZATION VERSION
echo "Building binary voxelization version ..."
${COMPILE_BINARY} tri_convert.cpp
${LINK_BINARY} tri_convert.o ${LINK_OPTS}

## CLEAN
echo "Cleaning..."
rm *.o
rm a.out

## BUILD REGULAR VERSION
echo "Building regular version ..."
${COMPILE} tri_convert.cpp
${LINK} tri_convert.o ${LINK_OPTS}

echo "Done"
