cmake_minimum_required(VERSION 3.22)
project(nezha-agent-cpp)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src/include)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message("This is a Windows system, Using MSVC compiler.")
    include(windows.cmake)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message("This is a Linux system, Using GCC compiler")
    include(linux.cmake)
else()
    message("Unknown compiler:" ${CMAKE_CXX_COMPILER_ID} )
endif()

find_package(protobuf REQUIRED)
get_target_property(PROTOBUF_INCLUDE_DIRS protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(PROTOBUF_LINK_LIBRARIES protobuf::libprotobuf INTERFACE_LINK_LIBRARIES)
include_directories(${PROTOBUF_INCLUDE_DIRS})

find_package(JsonCpp REQUIRED)
get_target_property(JSONCPP_INCLUDE_DIRS jsoncpp_static INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${JSONCPP_INCLUDE_DIRS})

find_package(CURL REQUIRED)
get_target_property(CUR_INCLUDE_DIRS CURL::libcurl INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(CUR_LINK_LIBRARIES CURL::libcurl INTERFACE_LINK_LIBRARIES)
include_directories(${CUR_INCLUDE_DIRS})

find_package(gRPC CONFIG REQUIRED)