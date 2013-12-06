cmake_minimum_required(VERSION 2.8)

project(rxcpp)

MESSAGE( STATUS "CMAKE_CXX_COMPILER_ID: " ${CMAKE_CXX_COMPILER_ID} )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list( APPEND CMAKE_CXX_FLAGS " -std=c++0x ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list( APPEND CMAKE_CXX_FLAGS " -std=c++0x ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    list( APPEND CMAKE_CXX_FLAGS " /DUNICODE /D_UNICODE /bigobj ${CMAKE_CXX_FLAGS}")
endif()

# define some folders
get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
set(TEST_DIR ${RXCPP_DIR}/Rx/CPP/test)

include_directories(${RXCPP_DIR}/Ix/CPP/src ${RXCPP_DIR}/Rx/CPP/src)

# define the sources of testbench
set(TESTBENCH_SOURCES
    ${RXCPP_DIR}/Rx/CPP/testbench/testbench.cpp
)
add_executable(testbench ${TESTBENCH_SOURCES})
