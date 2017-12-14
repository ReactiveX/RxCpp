cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcpp_test LANGUAGES C CXX)

# define some folders

get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

include(${RXCPP_DIR}/projects/CMake/shared.cmake)

# configure unit tests via CTest
enable_testing()

set(TEST_DIR ${RXCPP_DIR}/Rx/v2/test)

# define the sources of the self test
set(TEST_SOURCES
    ${TEST_DIR}/subscriptions/coroutine.cpp
    ${TEST_DIR}/subscriptions/observer.cpp
    ${TEST_DIR}/subscriptions/subscription.cpp
    ${TEST_DIR}/subjects/subject.cpp
    ${TEST_DIR}/sources/create.cpp
    ${TEST_DIR}/sources/defer.cpp
    ${TEST_DIR}/sources/empty.cpp
    ${TEST_DIR}/sources/interval.cpp
    ${TEST_DIR}/sources/scope.cpp
    ${TEST_DIR}/sources/timer.cpp
    ${TEST_DIR}/operators/all.cpp
    ${TEST_DIR}/operators/any.cpp
    ${TEST_DIR}/operators/amb.cpp
    ${TEST_DIR}/operators/amb_variadic.cpp
    ${TEST_DIR}/operators/buffer.cpp
    ${TEST_DIR}/operators/combine_latest.cpp
    ${TEST_DIR}/operators/concat.cpp
    ${TEST_DIR}/operators/concat_map.cpp
    ${TEST_DIR}/operators/contains.cpp
    ${TEST_DIR}/operators/debounce.cpp
    ${TEST_DIR}/operators/default_if_empty.cpp
    ${TEST_DIR}/operators/delay.cpp
    ${TEST_DIR}/operators/distinct.cpp
    ${TEST_DIR}/operators/distinct_until_changed.cpp
    ${TEST_DIR}/operators/element_at.cpp
    ${TEST_DIR}/operators/exists.cpp
    ${TEST_DIR}/operators/filter.cpp
    ${TEST_DIR}/operators/finally.cpp
    ${TEST_DIR}/operators/flat_map.cpp
    ${TEST_DIR}/operators/group_by.cpp
    ${TEST_DIR}/operators/ignore_elements.cpp
    ${TEST_DIR}/operators/is_empty.cpp
    ${TEST_DIR}/operators/lift.cpp
    ${TEST_DIR}/operators/map.cpp
    ${TEST_DIR}/operators/merge.cpp
    ${TEST_DIR}/operators/merge_delay_error.cpp
    ${TEST_DIR}/operators/observe_on.cpp
    ${TEST_DIR}/operators/on_error_resume_next.cpp
    ${TEST_DIR}/operators/pairwise.cpp
    ${TEST_DIR}/operators/publish.cpp
    ${TEST_DIR}/operators/reduce.cpp
    ${TEST_DIR}/operators/repeat.cpp
    ${TEST_DIR}/operators/replay.cpp
    ${TEST_DIR}/operators/retry.cpp
    ${TEST_DIR}/operators/sample.cpp
    ${TEST_DIR}/operators/scan.cpp
    ${TEST_DIR}/operators/sequence_equal.cpp
    ${TEST_DIR}/operators/skip.cpp
    ${TEST_DIR}/operators/skip_while.cpp
    ${TEST_DIR}/operators/skip_last.cpp
    ${TEST_DIR}/operators/skip_until.cpp
    ${TEST_DIR}/operators/start_with.cpp
    ${TEST_DIR}/operators/subscribe_on.cpp
    ${TEST_DIR}/operators/switch_if_empty.cpp
    ${TEST_DIR}/operators/switch_on_next.cpp
    ${TEST_DIR}/operators/take.cpp
    ${TEST_DIR}/operators/take_last.cpp
    ${TEST_DIR}/operators/take_until.cpp
    ${TEST_DIR}/operators/take_while.cpp
    ${TEST_DIR}/operators/tap.cpp
    ${TEST_DIR}/operators/time_interval.cpp
    ${TEST_DIR}/operators/timeout.cpp
    ${TEST_DIR}/operators/timestamp.cpp
    ${TEST_DIR}/operators/with_latest_from.cpp
    ${TEST_DIR}/operators/window.cpp
    ${TEST_DIR}/operators/window_toggle.cpp
    ${TEST_DIR}/operators/zip.cpp
)


add_executable(rxcppv2_test ${TEST_DIR}/test.cpp ${TEST_SOURCES})
add_executable(rxcpp::tests ALIAS rxcppv2_test)
set_target_properties(
    rxcppv2_test PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD TRUE
    EXCLUDE_FROM_ALL TRUE
)
target_compile_options(rxcppv2_test PUBLIC ${RX_COMPILE_OPTIONS})
target_compile_features(rxcppv2_test PUBLIC ${RX_COMPILE_FEATURES})
target_include_directories(rxcppv2_test
    PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
    )
target_link_libraries(rxcppv2_test ${CMAKE_THREAD_LIBS_INIT})


foreach(ONE_TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(ONE_TEST_NAME "${ONE_TEST_SOURCE}" NAME)
    string( REPLACE ".cpp" "" ONE_TEST_NAME ${ONE_TEST_NAME})
    set(ONE_TEST_FULL_NAME "rxcpp_test_${ONE_TEST_NAME}")
    add_executable( ${ONE_TEST_FULL_NAME} ${ONE_TEST_SOURCE} )
    add_executable( rxcpp::${ONE_TEST_NAME} ALIAS ${ONE_TEST_FULL_NAME})
    target_compile_definitions(${ONE_TEST_FULL_NAME} PUBLIC "CATCH_CONFIG_MAIN")
    target_compile_options(${ONE_TEST_FULL_NAME} PUBLIC ${RX_COMPILE_OPTIONS})
    target_compile_features(${ONE_TEST_FULL_NAME} PUBLIC ${RX_COMPILE_FEATURES})
    target_include_directories(${ONE_TEST_FULL_NAME}
        PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
        )
    target_link_libraries(${ONE_TEST_FULL_NAME} ${CMAKE_THREAD_LIBS_INIT})

    add_test(NAME ${ONE_TEST_NAME} COMMAND ${ONE_TEST_FULL_NAME})
endforeach(ONE_TEST_SOURCE ${TEST_SOURCES})



