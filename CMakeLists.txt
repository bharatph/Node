cmake_minimum_required(VERSION 3.0)
project(Node VERSION 0.5 LANGUAGES CXX DESCRIPTION "Asynchronous Networking library for C++")
file(GLOB_RECURSE Node_HDRS "include/*hpp")
file(GLOB_RECURSE Node_SRCS "src/*.cpp")

configure_file( ${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_SOURCE_DIR}/config.h)

add_library(Node ${Node_HDRS} ${Node_SRCS})
target_compile_features(Node PRIVATE cxx_enum_forward_declarations)
if(WIN32)
	target_link_libraries(Node wsock32 ws2_32)
endif()
set_target_properties(Node PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(Node PROPERTIES SOVERSION 1)
set_target_properties(Node PROPERTIES PUBLIC_HEADER ${Node_HDRS})
target_include_directories(Node PUBLIC include)

install(FILES ${Node_HDRS} DESTINATION include/Node)

install (TARGETS Node
	PUBLIC_HEADER DESTINATION include/Node
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin)
