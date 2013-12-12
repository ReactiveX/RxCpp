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

include_directories(${RXCPP_DIR}/ext/catch/include ${RXCPP_DIR}/Ix/CPP/src ${RXCPP_DIR}/Rx/CPP/src)

# define the sources of the self test
set(TEST_SOURCES
    ${TEST_DIR}/test.cpp
    ${TEST_DIR}/operators/Return.cpp
    ${TEST_DIR}/operators/Select.cpp
    ${TEST_DIR}/operators/Where.cpp
    ${TEST_DIR}/operators/Publish.cpp
)
add_executable(rxcpp_test ${TEST_SOURCES})

# define the sources of testbench
set(TESTBENCH_SOURCES
    ${RXCPP_DIR}/Rx/CPP/testbench/testbench.cpp
)
add_executable(testbench ${TESTBENCH_SOURCES})

# configure unit tests via CTest
enable_testing()
add_test(NAME RunTests COMMAND rxcpp_test)

add_test(NAME ListTests COMMAND rxcpp_test --list-tests)
set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")

add_test(NAME ListTags COMMAND rxcpp_test --list-tags)
set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")
