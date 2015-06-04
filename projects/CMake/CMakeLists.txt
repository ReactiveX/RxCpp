cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcpp VERSION 2.2.4 LANGUAGES C CXX)

# define some folders

get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

add_subdirectory(${RXCPP_DIR}/Rx/v2/test ${CMAKE_CURRENT_BINARY_DIR}/test)

add_subdirectory(${RXCPP_DIR}/projects/doxygen ${CMAKE_CURRENT_BINARY_DIR}/projects/doxygen)

set(EXAMPLES_DIR ${RXCPP_DIR}/Rx/v2/examples)

add_subdirectory(${EXAMPLES_DIR}/cep ${CMAKE_CURRENT_BINARY_DIR}/examples/cep)
add_subdirectory(${EXAMPLES_DIR}/stop ${CMAKE_CURRENT_BINARY_DIR}/examples/stop)
add_subdirectory(${EXAMPLES_DIR}/println ${CMAKE_CURRENT_BINARY_DIR}/examples/println)
add_subdirectory(${EXAMPLES_DIR}/pythagorian ${CMAKE_CURRENT_BINARY_DIR}/examples/pythagorian)
add_subdirectory(${EXAMPLES_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/examples/tests)
