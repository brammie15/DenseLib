cmake_minimum_required(VERSION 3.31)
project(DenseLib)
include(FetchContent)

set(source_files
    src/main.cpp
    src/tiny_obj_loader.h

    src/IO.h src/IO.cpp
    src/Chunk.h src/Chunk.cpp
    src/Format.h src/Format.cpp
    src/AssetWriter.h src/AssetWriter.cpp
    src/AssetReader.h src/AssetReader.cpp
)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} ${source_files})

set(ZLIB_TEST ON)
FetchContent_Declare(
        zlib-cmake
        URL https://github.com/jimmy-park/zlib-cmake/archive/main.tar.gz
)
FetchContent_MakeAvailable(zlib-cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)

add_custom_target(copy_resources ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "${CMAKE_BINARY_DIR}/resources"
        COMMENT "Copying resources folder to build directory"
)

add_dependencies(${PROJECT_NAME} copy_resources)

