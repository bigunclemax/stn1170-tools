cmake_minimum_required(VERSION 3.10)

project(stntool)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_BINARY_DIR})
include_directories(cxxopts/include)

add_subdirectory(serial)

add_library(stnlib stnlib/SerialPort.cpp)
target_include_directories(stnlib PRIVATE ./serial/include)
target_compile_definitions(stnlib PRIVATE DEBUG DEBUG_PRINT_HEX)

add_executable(${CMAKE_PROJECT_NAME} utils/stntool.cpp)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ./stnlib ./serial/include)
target_link_libraries(${CMAKE_PROJECT_NAME} stnlib serial)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Werror=vla)
endif()