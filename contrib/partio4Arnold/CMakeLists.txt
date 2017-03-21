cmake_minimum_required(VERSION 2.4.6)

#if (BUILD_PARTIO_MTOA)
	find_package(MtoA)
#endif ()

########################################################################## HDA
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : BUILD_PARTIO_MTOA  = ${BUILD_PARTIO_MTOA}")
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : MTOA_FOUND  = ${MTOA_FOUND}")
message(STATUS  "partio/contrib/partio4Arnold/CMakeLists.txt : ARNOLD_FOUND  = ${ARNOLD_FOUND}")
########################################################################## HDA


#if(MTOA_FOUND)
#	add_subdirectory(plugin)
#else()
#	add_subdirectory(procedurals)
#endif()

########################################################################## HDA

if(MTOA_FOUND)
	add_subdirectory(plugin)
endif()

if(ARNOLD_FOUND)
	add_subdirectory(procedurals)
endif()
########################################################################## HDA
