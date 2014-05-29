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

set(V2_TEST_DIR ${RXCPP_DIR}/Rx/v2/test)

include_directories(${RXCPP_DIR}/ext/catch/include ${RXCPP_DIR}/Ix/CPP/src ${RXCPP_DIR}/Rx/CPP/src ${RXCPP_DIR}/Rx/v2/src)

# define the sources of the self test
set(TEST_SOURCES
    ${TEST_DIR}/test.cpp
    ${TEST_DIR}/operators/Merge.cpp
    ${TEST_DIR}/operators/Return.cpp
    ${TEST_DIR}/operators/Select.cpp
    ${TEST_DIR}/operators/SelectMany.cpp
    ${TEST_DIR}/operators/Where.cpp
    ${TEST_DIR}/operators/Publish.cpp
)
add_executable(rxcpp_test ${TEST_SOURCES})

# define the sources of testbench
set(TESTBENCH_SOURCES
    ${RXCPP_DIR}/Rx/CPP/testbench/testbench.cpp
)
add_executable(testbench ${TESTBENCH_SOURCES})

# define the sources of the self test
set(V2_TEST_SOURCES
    ${V2_TEST_DIR}/test.cpp
    ${V2_TEST_DIR}/operators/merge.cpp
    ${V2_TEST_DIR}/operators/lift.cpp
    ${V2_TEST_DIR}/operators/filter.cpp
    ${V2_TEST_DIR}/subscriptions/observer.cpp
    ${V2_TEST_DIR}/operators/take.cpp
    ${V2_TEST_DIR}/operators/publish.cpp
    ${V2_TEST_DIR}/subjects/subject.cpp
    ${V2_TEST_DIR}/subscriptions/subscription.cpp
    ${V2_TEST_DIR}/operators/flat_map.cpp
    ${V2_TEST_DIR}/operators/map.cpp
)
add_executable(rxcppv2_test ${V2_TEST_SOURCES})

# configure unit tests via CTest
enable_testing()

add_test(NAME RunTests COMMAND rxcpp_test)

add_test(NAME ListTests COMMAND rxcpp_test --list-tests)
set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")

add_test(NAME ListTags COMMAND rxcpp_test --list-tags)
set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")

add_test(NAME RunTestsV2 COMMAND rxcppv2_test)

add_test(NAME ListTestsV2 COMMAND rxcppv2_test --list-tests)
set_tests_properties(ListTestsV2 PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")

add_test(NAME ListTagsV2 COMMAND rxcppv2_test --list-tags)
set_tests_properties(ListTagsV2 PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")
