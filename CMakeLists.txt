cmake_minimum_required(VERSION 3.0)
project(Node VERSION 0.5 DESCRIPTION "Asynchronous Networking library for C++")
file(GLOB_RECURSE Node_HDRS "include/*.hpp")
file(GLOB_RECURSE Node_SRCS "src/*.cpp")

configure_file( ${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_SOURCE_DIR}/config.h)

add_library(Node SHARED ${Node_HDRS} ${Node_SRCS})
set_target_properties(Node PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(Node PROPERTIES SOVERSION 1)
set_target_properties(Node PROPERTIES PUBLIC_HEADER ${Node_HDRS})

target_include_directories(Node PUBLIC include)
if(ENABLE_CLOG)
	target_link_libraries(Node clog)
endif()