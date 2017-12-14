cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(doxygen LANGUAGES C CXX)

# define some folders

get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

# target to generate documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
    include(${RXCPP_DIR}/projects/CMake/shared.cmake)
    set(EXAMPLES_DIR ${RXCPP_DIR}/Rx/v2/examples)

    # Doxygen configuration
    set(DOXY_CONF_DIR ${RXCPP_DIR}/projects/doxygen)
    set(DOXY_CONF_FILE ${DOXY_CONF_DIR}/doxygen.conf)

    # File with main page (index.html) content
    set(DOXY_MAIN_PAGE ${DOXY_CONF_DIR}/mainpage.dox)
    set(DOXY_FOOTER_HTML ${DOXY_CONF_DIR}/footer.html)

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
        ${DOXY_EXAMPLES_SRC_DIR}/all.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/amb.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/as_dynamic.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/blocking_observable.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/buffer.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/combine_latest.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/composite_exception.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/concat.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/concat_map.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/contains.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/create.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/defer.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/debounce.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/default_if_empty.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/delay.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/distinct.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/distinct_until_changed.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/element_at.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/empty.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/error.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/exists.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/filter.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/finally.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/flat_map.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/from.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/group_by.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/ignore_elements.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/interval.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/iterate.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/just.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/map.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/math.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/merge.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/merge_delay_error.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/never.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/observe_on.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/on_error_resume_next.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/pairwise.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/publish.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/range.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/reduce.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/repeat.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/replay.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/retry.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/sample.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/scan.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/scope.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/sequence_equal.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/skip.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/skip_while.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/skip_last.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/skip_until.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/start_with.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/subscribe.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/subscribe_on.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/switch_if_empty.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/switch_on_next.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/take.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/take_last.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/take_until.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/take_while.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/tap.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/time_interval.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/timeout.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/timer.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/timestamp.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/with_latest_from.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/window.cpp
        ${DOXY_EXAMPLES_SRC_DIR}/zip.cpp
    )
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DOXY_EXAMPLES_BIN_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DOXY_EXAMPLES_BIN_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DOXY_EXAMPLES_BIN_DIR})
    add_executable(doxy_examples EXCLUDE_FROM_ALL ${DOXY_EXAMPLE_SRC_LIST})
    add_executable(rxcpp::doxygen::examples ALIAS doxy_examples)
    target_compile_options(doxy_examples PUBLIC ${RX_COMPILE_OPTIONS})
    target_compile_features(doxy_examples PUBLIC ${RX_COMPILE_FEATURES})
    target_include_directories(doxy_examples
        PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
        )
    target_link_libraries(doxy_examples PRIVATE ${CMAKE_THREAD_LIBS_INIT})

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
