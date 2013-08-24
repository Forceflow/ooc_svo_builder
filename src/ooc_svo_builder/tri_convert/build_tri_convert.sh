
## SPECIFY TRIMESH LOCATION HERE (and do a make there first)
TRIMESH_DIR=/home/jeroen/Downloads/trimesh2

## COMPILE
rm *.o
rm tri_convert
rm a.out
g++ -c -I ../tri_tools/include/ -I ${TRIMESH_DIR}/include/ tri_convert.cpp
g++ -o tri_convert tri_convert.o -L${TRIMESH_DIR}/lib.Linux64 -ltrimesh -fopenmp -static -o tri_convert
