cmake_minimum_required(VERSION 3.13)
project(SophiesDiagramGenerator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(SophiesDiagramGenerator main.cpp)

find_package(Cairo)
include_directories(${CAIRO_INCLUDE_DIR})
target_link_libraries(SophiesDiagramGenerator ${CAIRO_LIBRARIES})

# apt install build-essential libcairo2-dev
