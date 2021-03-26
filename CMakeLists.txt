cmake_minimum_required(VERSION 3.13)
project(SophiesDiagramGenerator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(SophiesDiagramGenerator main.cpp)

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries(SophiesDiagramGenerator ${LIBXML2_LIBRARY})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIR})
target_link_libraries(SophiesDiagramGenerator ${CAIRO_LIBRARIES})

# apt install build-essential libcairo2-dev libxml2-dev
