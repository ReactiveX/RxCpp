cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

get_filename_component(SAMPLE_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

project(${SAMPLE_PROJECT} LANGUAGES C CXX)

# define some folders
get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

include(${RXCPP_DIR}/projects/CMake/shared.cmake)

# define the sources
set(SAMPLE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
add_executable(${SAMPLE_PROJECT} ${SAMPLE_SOURCES})
add_executable(rxcpp::examples::${SAMPLE_PROJECT} ALIAS ${SAMPLE_PROJECT})
target_compile_options(${SAMPLE_PROJECT} PUBLIC ${RX_COMPILE_OPTIONS})
target_compile_features(${SAMPLE_PROJECT} PUBLIC ${RX_COMPILE_FEATURES})
target_include_directories(${SAMPLE_PROJECT}
    PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
    )
target_link_libraries(${SAMPLE_PROJECT} ${CMAKE_THREAD_LIBS_INIT})

# configure unit tests via CTest
enable_testing()
set(CTEST_CONFIGURATION_TYPE "${JOB_BUILD_CONFIGURATION}")

set_target_properties(${SAMPLE_PROJECT} PROPERTIES FOLDER "Examples")

add_test(NAME RunTests
     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
     COMMAND ${SAMPLE_PROJECT} ${TEST_ARGS})
