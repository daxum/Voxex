cmake_minimum_required(VERSION 3.1)
project(ChunkRenderTest)

set(ENGINE_DIR "" CACHE STRING "Engine repository directory")

if ("${ENGINE_DIR}" STREQUAL "")
	message(FATAL_ERROR "Engine directory not set")
endif()

add_subdirectory("${ENGINE_DIR}" ${CMAKE_CURRENT_BINARY_DIR}/engine)

#TODO: remove once merged with engine
find_package(glfw3 REQUIRED)

add_executable(chunkTest
	Main.cpp
	RegionTree.cpp
	Chunk.cpp
	Voxex.cpp
	Perlin.cpp
	SquareCamera.cpp
	ControlledAI.cpp
)

set_target_properties(chunkTest PROPERTIES
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED ON
)

if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	target_compile_options(chunkTest PRIVATE "-Wall")
endif()

target_link_libraries(chunkTest Engine glfw GL tbb)

add_custom_command(
	TARGET chunkTest POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/../resources $<TARGET_FILE_DIR:chunkTest>
)
