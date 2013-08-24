CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( OutOfCoreSparseVoxelOctree )

FIND_PACKAGE ( OpenMP REQUIRED )

IF (MSVC)
  ADD_DEFINITIONS ( -openmp )
ELSE ()
  # Apple and Linux both uses GCC
  ADD_DEFINITIONS ( -fopenmp )
ENDIF (MSVC)

INCLUDE_DIRECTORIES (
  ./src/ooc_svo_builder/tri_convert
  ./src/ooc_svo_builder/tri_tools/include
  )

INCLUDE_DIRECTORIES (
  ${TRIMESH2_ROOT}/include
)

LINK_DIRECTORIES (
  ${TRIMESH2_ROOT}/lib.Darwin64
)

ADD_EXECUTABLE ( svo_builder
  ./src/ooc_svo_builder/svo_builder/main.cpp
  ./src/ooc_svo_builder/svo_builder/OctreeBuilder.cpp
  ./src/ooc_svo_builder/svo_builder/partitioner.cpp
  ./src/ooc_svo_builder/svo_builder/voxelizer.cpp
  )

ADD_EXECUTABLE ( tri_convert
  ./src/ooc_svo_builder/tri_convert/tri_convert.cpp
  )

TARGET_LINK_LIBRARIES ( svo_builder
  gomp
)

TARGET_LINK_LIBRARIES ( tri_convert
  trimesh
  gomp
)
