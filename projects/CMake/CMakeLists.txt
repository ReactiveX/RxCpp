cmake_minimum_required(VERSION 2.8)

project(rxcpp)

MESSAGE( STATUS "CMAKE_CXX_COMPILER_ID: " ${CMAKE_CXX_COMPILER_ID} )

FIND_PACKAGE(Threads)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list( APPEND CMAKE_CXX_FLAGS " -std=c++11 ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list( APPEND CMAKE_CXX_FLAGS " -std=c++11 ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    list( APPEND CMAKE_CXX_FLAGS " /DUNICODE /D_UNICODE /bigobj ${CMAKE_CXX_FLAGS}")
endif()


# define some folders
get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )


include_directories(${RXCPP_DIR}/ext/catch/include ${RXCPP_DIR}/Ix/CPP/src ${RXCPP_DIR}/Rx/v2/src)


set(TEST_DIR ${RXCPP_DIR}/Rx/v2/test)

# define the sources of the self test
set(TEST_SOURCES
    ${TEST_DIR}/test.cpp
    ${TEST_DIR}/subscriptions/observer.cpp
    ${TEST_DIR}/subscriptions/subscription.cpp
    ${TEST_DIR}/subjects/subject.cpp
    ${TEST_DIR}/sources/defer.cpp
    ${TEST_DIR}/sources/interval.cpp
    ${TEST_DIR}/operators/buffer.cpp
    ${TEST_DIR}/operators/combine_latest.1.cpp
    ${TEST_DIR}/operators/combine_latest.2.cpp
    ${TEST_DIR}/operators/concat.cpp
    ${TEST_DIR}/operators/concat_map.cpp
    ${TEST_DIR}/operators/distinct_until_changed.cpp
    ${TEST_DIR}/operators/filter.cpp
    ${TEST_DIR}/operators/flat_map.cpp
    ${TEST_DIR}/operators/lift.cpp
    ${TEST_DIR}/operators/map.cpp
    ${TEST_DIR}/operators/merge.cpp
    ${TEST_DIR}/operators/observe_on.cpp
    ${TEST_DIR}/operators/publish.cpp
    ${TEST_DIR}/operators/reduce.cpp
    ${TEST_DIR}/operators/repeat.cpp
    ${TEST_DIR}/operators/scan.cpp
    ${TEST_DIR}/operators/skip.cpp
    ${TEST_DIR}/operators/skip_until.cpp
    ${TEST_DIR}/operators/switch_on_next.cpp
    ${TEST_DIR}/operators/take.cpp
    ${TEST_DIR}/operators/take_until.cpp
)
add_executable(rxcppv2_test ${TEST_SOURCES})
TARGET_LINK_LIBRARIES(rxcppv2_test ${CMAKE_THREAD_LIBS_INIT})

set(EXAMPLES_DIR ${RXCPP_DIR}/Rx/v2/examples)

# define the sources of the pythagorian example
set(PYTHAGORIAN_SOURCES
    ${EXAMPLES_DIR}/pythagorian/main.cpp
)
add_executable(pythagorian ${PYTHAGORIAN_SOURCES})
TARGET_LINK_LIBRARIES(pythagorian ${CMAKE_THREAD_LIBS_INIT})

# define the sources of the pythagorian example
set(PRINTLN_SOURCES
    ${EXAMPLES_DIR}/println/main.cpp
)
add_executable(println ${PRINTLN_SOURCES})
TARGET_LINK_LIBRARIES(println ${CMAKE_THREAD_LIBS_INIT})

# configure unit tests via CTest
enable_testing()

add_test(NAME RunTests COMMAND rxcppv2_test)

add_test(NAME ListTests COMMAND rxcppv2_test --list-tests)
set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")

add_test(NAME ListTags COMMAND rxcppv2_test --list-tags)
set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")
