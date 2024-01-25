find_package(unofficial-getopt-win32 REQUIRED)

add_definitions(-DSPDLOG_WCHAR_TO_UTF8_SUPPORT)
add_definitions(-DSPDLOG_WCHAR_FILENAMES)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-D_AMD64_)

add_compile_options(/W0)
add_compile_options(/utf-8)
add_compile_options(/MP)
add_compile_options($<$<CONFIG:Release>:/Gy>)
add_compile_options($<$<CONFIG:Release>:/Oi>)
add_compile_options($<$<CONFIG:Release>:/Zi>)
add_compile_options($<$<CONFIG:Release>:/EHsc>)
add_compile_options($<$<CONFIG:Release>:/MT>)
add_compile_options($<$<CONFIG:Debug>:/MTd>)
add_compile_options($<$<CONFIG:Debug>:/EHa>)

add_link_options($<$<CONFIG:Release>:/OPT:REF>)
add_link_options($<$<CONFIG:Release>:/OPT:ICF>)
add_link_options($<$<CONFIG:Release>:/DEBUG>)

file(GLOB_RECURSE PROTO_SRC
        ${PROJECT_SOURCE_DIR}/src/proto/*.h
        ${PROJECT_SOURCE_DIR}/src/proto/*.cc
)

set(EXE_NAME "nezha-agent-cpp_win")
if(CMAKE_CL_64)
    message("64-bit system")
    set(EXE_NAME "${EXE_NAME}_x64")
else()
    message("32-bit system")
    set(EXE_NAME "${EXE_NAME}_x86")
endif()

add_executable(${EXE_NAME}
        ${PROTO_SRC}
        src/main.cpp
        src/nezha_util.cpp
        src/IPManager.cpp
        src/machine_info/MachineInfo_win.cpp
)

target_link_libraries(${EXE_NAME} PRIVATE
        gRPC::gpr
        gRPC::grpc
        gRPC::grpc++
        gRPC::grpc++_alts
        protobuf::libprotobuf
        unofficial::getopt-win32::getopt
        CURL::libcurl
        jsoncpp_static
)