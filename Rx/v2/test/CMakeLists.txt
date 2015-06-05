cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcppv2_test LANGUAGES C CXX)

# define some folders

get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

include(${RXCPP_DIR}/projects/CMake/shared.cmake)

set(TEST_DIR ${RXCPP_DIR}/Rx/v2/test)

# define the sources of the self test
set(TEST_SOURCES
    ${TEST_DIR}/test.cpp
    ${TEST_DIR}/subscriptions/observer.cpp
    ${TEST_DIR}/subscriptions/subscription.cpp
    ${TEST_DIR}/subjects/subject.cpp
    ${TEST_DIR}/sources/create.cpp
    ${TEST_DIR}/sources/defer.cpp
    ${TEST_DIR}/sources/interval.cpp
    ${TEST_DIR}/sources/scope.cpp
    ${TEST_DIR}/sources/timer.cpp
    ${TEST_DIR}/operators/amb.cpp
    ${TEST_DIR}/operators/amb_variadic.cpp
    ${TEST_DIR}/operators/buffer.cpp
    ${TEST_DIR}/operators/combine_latest.1.cpp
    ${TEST_DIR}/operators/combine_latest.2.cpp
    ${TEST_DIR}/operators/concat.cpp
    ${TEST_DIR}/operators/concat_map.cpp
    ${TEST_DIR}/operators/distinct_until_changed.cpp
    ${TEST_DIR}/operators/filter.cpp
    ${TEST_DIR}/operators/flat_map.cpp
    ${TEST_DIR}/operators/group_by.cpp
    ${TEST_DIR}/operators/lift.cpp
    ${TEST_DIR}/operators/map.cpp
    ${TEST_DIR}/operators/merge.cpp
    ${TEST_DIR}/operators/observe_on.cpp
    ${TEST_DIR}/operators/pairwise.cpp
    ${TEST_DIR}/operators/publish.cpp
    ${TEST_DIR}/operators/reduce.cpp
    ${TEST_DIR}/operators/repeat.cpp
    ${TEST_DIR}/operators/retry.cpp
    ${TEST_DIR}/operators/scan.cpp
    ${TEST_DIR}/operators/skip.cpp
    ${TEST_DIR}/operators/skip_until.cpp
    ${TEST_DIR}/operators/subscribe_on.cpp
    ${TEST_DIR}/operators/switch_on_next.cpp
    ${TEST_DIR}/operators/take.cpp
    ${TEST_DIR}/operators/take_until.cpp
    ${TEST_DIR}/operators/window.cpp
    ${TEST_DIR}/operators/zip.1.cpp
    ${TEST_DIR}/operators/zip.2.cpp
)
add_executable(rxcppv2_test ${TEST_SOURCES})
add_executable(rxcpp::tests ALIAS rxcppv2_test)
target_compile_options(rxcppv2_test PUBLIC ${RX_COMPILE_OPTIONS})
target_compile_features(rxcppv2_test PUBLIC ${RX_COMPILE_FEATURES})
target_include_directories(rxcppv2_test
    PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
    )
target_link_libraries(rxcppv2_test ${CMAKE_THREAD_LIBS_INIT})

# define the sources of the self test
set(ONE_SOURCES
    ${TEST_DIR}/test.cpp
    #${TEST_DIR}/operators/combine_latest.1.cpp
    #${TEST_DIR}/operators/combine_latest.2.cpp
)
add_executable(one_test ${ONE_SOURCES})
add_executable(rxcpp::one_test ALIAS one_test)
target_compile_options(one_test PUBLIC ${RX_COMPILE_OPTIONS})
target_compile_features(one_test PUBLIC ${RX_COMPILE_FEATURES})
target_include_directories(one_test
    PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
    )
target_link_libraries(one_test ${CMAKE_THREAD_LIBS_INIT})

# configure unit tests via CTest
enable_testing()

add_test(NAME RunTests COMMAND rxcppv2_test)

add_test(NAME ListTests COMMAND rxcppv2_test --list-tests)
set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")

add_test(NAME ListTags COMMAND rxcppv2_test --list-tags)
set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")

