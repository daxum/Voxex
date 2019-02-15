cmake_minimum_required(VERSION 3.1)
project(Voxex)

set(ENGINE_DIR "" CACHE STRING "Engine repository directory")

if ("${ENGINE_DIR}" STREQUAL "")
	message(FATAL_ERROR "Engine directory not set")
endif()

add_subdirectory("${ENGINE_DIR}" ${CMAKE_CURRENT_BINARY_DIR}/engine)

add_executable(voxex
	Main.cpp
	RegionTree.cpp
	Chunk.cpp
	Voxex.cpp
	Perlin.cpp
)

set_target_properties(voxex PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
)

if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	target_compile_options(voxex PRIVATE "-Wall")
endif()

target_link_libraries(voxex Engine)

add_custom_command(
	TARGET voxex POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/../resources $<TARGET_FILE_DIR:voxex>
)
