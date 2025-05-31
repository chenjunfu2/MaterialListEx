cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

project(MaterialListEx)

find_package(ZLIB REQUIRED)

find_path(GZIP_HPP_INCLUDE_DIRS "gzip/compress.hpp")

find_package(nlohmann_json CONFIG REQUIRED)

find_package(xxHash CONFIG REQUIRED)

add_executable(MaterialListEx
    MaterialListEx/main.cpp
    MaterialListEx/RegionProcess.cpp)
target_include_directories(MaterialListEx PUBLIC
    MaterialListEx
    ${GZIP_HPP_INCLUDE_DIRS})
target_link_libraries(MaterialListEx PRIVATE
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
    xxHash::xxhash)

