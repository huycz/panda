cmake_minimum_required(VERSION 2.8)
project(panda)
execute_process(COMMAND uname
                OUTPUT_VARIABLE UNAME_INFO
                OUTPUT_STRIP_TRAILING_WHITESPACE)
message("uname info: '${UNAME_INFO}'")
include(configure/CheckSystemFunctions.cmake)
include(configure/ConfigSystemInfo.cmake.${UNAME_INFO})
include_directories(inc)
add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(example)
