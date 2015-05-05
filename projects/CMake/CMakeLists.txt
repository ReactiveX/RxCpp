cmake_minimum_required(VERSION 2.8)

project(rxcpp)

FIND_PACKAGE(Threads)

MESSAGE( STATUS "CMAKE_CXX_COMPILER_ID: " ${CMAKE_CXX_COMPILER_ID} )
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    MESSAGE( STATUS "using clang settings" )
    add_compile_options( -Wall -Wextra -Werror )
    add_compile_options( -std=c++11 -stdlib=libc++ )
    add_compile_options( -ftemplate-depth=1024 ) # sometimes you just do what the compiler tells you
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    MESSAGE( STATUS "using gnu settings" )
    add_compile_options( -Wall -Wextra -Werror )
    add_compile_options( -std=c++11 )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    MESSAGE( STATUS "using msvc settings" )
    add_compile_options( /W4 /WX )
    add_compile_options( /wd4503 ) # truncated symbol
    add_compile_options( /wd4702 ) # unreachable code
    add_compile_options( /bigobj )
    add_definitions( /DUNICODE /D_UNICODE ) # it is a new millenium
endif()


# define some folders
get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

set(IX_SRC_DIR ${RXCPP_DIR}/Ix/CPP/src)
set(RX_SRC_DIR ${RXCPP_DIR}/Rx/v2/src)

include_directories(SYSTEM ${RXCPP_DIR}/ext/catch/include)
include_directories(${IX_SRC_DIR} ${RX_SRC_DIR})

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
TARGET_LINK_LIBRARIES(rxcppv2_test ${CMAKE_THREAD_LIBS_INIT})

# define the sources of the self test
set(ONE_SOURCES
    ${TEST_DIR}/test.cpp
    #${TEST_DIR}/operators/combine_latest.1.cpp
    #${TEST_DIR}/operators/combine_latest.2.cpp
)
add_executable(one_test ${ONE_SOURCES})
TARGET_LINK_LIBRARIES(one_test ${CMAKE_THREAD_LIBS_INIT})

set(EXAMPLES_DIR ${RXCPP_DIR}/Rx/v2/examples)

# define the sources of the pythagorian example
set(PYTHAGORIAN_SOURCES
    ${EXAMPLES_DIR}/pythagorian/main.cpp
)
add_executable(pythagorian ${PYTHAGORIAN_SOURCES})
TARGET_LINK_LIBRARIES(pythagorian ${CMAKE_THREAD_LIBS_INIT})

# define the sources of the println example
set(PRINTLN_SOURCES
    ${EXAMPLES_DIR}/println/main.cpp
)
add_executable(println ${PRINTLN_SOURCES})
TARGET_LINK_LIBRARIES(println ${CMAKE_THREAD_LIBS_INIT})

# define the sources of the tests example
set(TESTS_EXAMPLE_SOURCES
    ${EXAMPLES_DIR}/tests/main.cpp
    ${EXAMPLES_DIR}/tests/take.cpp
)
add_executable(tests_example ${TESTS_EXAMPLE_SOURCES})
TARGET_LINK_LIBRARIES(tests_example ${CMAKE_THREAD_LIBS_INIT})

# configure unit tests via CTest
enable_testing()

add_test(NAME RunTests COMMAND rxcppv2_test)

add_test(NAME ListTests COMMAND rxcppv2_test --list-tests)
set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test cases")

add_test(NAME ListTags COMMAND rxcppv2_test --list-tags)
set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")

# target to generate documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # Doxygen configuration
    set(DOXY_CONF_DIR ${RXCPP_DIR}/projects/doxygen)
    set(DOXY_CONF_FILE ${DOXY_CONF_DIR}/doxygen.conf)

    # File with main page (index.html) content
    set(DOXY_MAIN_PAGE ${DOXY_CONF_DIR}/mainpage.dox)

    # Sources for processing location
    set(DOXY_INPUT_DIR "${IX_SRC_DIR} ${RX_SRC_DIR} ${DOXY_MAIN_PAGE}")

    # C++ examples location
    set(DOXY_EXAMPLES_SRC_DIR ${EXAMPLES_DIR}/doxygen)
    set(DOXY_EXAMPLES_BIN_DIR doxy_examples)

    # Directories and files allowed for include-type command (\include, \example, \snippet etc)
    set(DOXY_INCLUDE_DIR "${RXCPP_DIR} ${DOXY_EXAMPLES_SRC_DIR}")

    # Resulting documentation to be placed here
    set(DOXY_OUTPUT_DIR ${RXCPP_DIR}/projects/doxygen)

    # Make Doxygen configuration from template with placeholders
    configure_file(${DOXY_CONF_FILE}.in ${DOXY_CONF_FILE})

    # Target to build examples
    set(DOXY_EXAMPLE_SRC_LIST
        ${DOXY_EXAMPLES_SRC_DIR}/main.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/create.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/defer.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/empty.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/error.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/from.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/interval.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/iterate.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/just.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/never.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/range.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/scope.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/start_with.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/timer.cpp
    )
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DOXY_EXAMPLES_BIN_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DOXY_EXAMPLES_BIN_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DOXY_EXAMPLES_BIN_DIR})
    add_executable(doxy_examples EXCLUDE_FROM_ALL ${DOXY_EXAMPLE_SRC_LIST})
    TARGET_LINK_LIBRARIES(doxy_examples ${CMAKE_THREAD_LIBS_INIT})

    # Target to execute examples and save their output
    add_custom_command(
        OUTPUT doxy_examples_output
        COMMAND ${DOXY_EXAMPLES_BIN_DIR}/doxy_examples > ${DOXY_EXAMPLES_SRC_DIR}/output.txt
        COMMENT "Execute examples for Doxygen documentation")

    # Target to generate docs
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXY_CONF_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS doxy_examples doxy_examples_output
        COMMENT "Generating documentation with Doxygen"
        VERBATIM)
endif()
