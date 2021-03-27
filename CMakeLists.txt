cmake_minimum_required(VERSION 3.13)
project(SophiesDiagramGenerator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(
    ${PROJECT_NAME}
    source/sdg.cpp
    source/xmlLoader.cpp
    source/cli.cpp
)

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries(SophiesDiagramGenerator ${LIBXML2_LIBRARY})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIR})
target_link_libraries(SophiesDiagramGenerator ${CAIRO_LIBRARIES})

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/example/example.xml ${CMAKE_CURRENT_SOURCE_DIR}/example/example.svg 9999
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/example/example.xml ${CMAKE_CURRENT_SOURCE_DIR}/example/example_lod2.svg 2
)
