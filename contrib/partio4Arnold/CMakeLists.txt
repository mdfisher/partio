cmake_minimum_required(VERSION 2.4.6)

if (BUILD_PARTIO_MTOA)
	find_package(MtoA)
endif ()

if(MTOA_FOUND)
	add_subdirectory(plugin)
else()
	add_subdirectory(procedurals)
endif()
