cmake_minimum_required(VERSION 3.10)

project(stntool)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_BINARY_DIR})
include_directories(cxxopts/include)

add_subdirectory(serial)

add_library(stnlib stnlib/CanDevice.cpp)
target_include_directories(stnlib PUBLIC ./serial/include)
target_link_libraries(stnlib serial)
#target_compile_definitions(stnlib PRIVATE DEBUG DEBUG_PRINT_HEX) #uncomment this if you need debug serial exchange

add_executable(stntool utils/stntool.cpp)
target_include_directories(stntool PRIVATE ./stnlib ./serial/include)
target_link_libraries(stntool stnlib serial)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Werror=vla)
endif()