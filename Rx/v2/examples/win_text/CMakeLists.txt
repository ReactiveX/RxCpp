cmake_minimum_required(VERSION 2.8)

get_filename_component(SAMPLE_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}" NAME)

project(${SAMPLE_PROJECT})

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
    add_compile_options( /wd4091 ) # typedef ignored on left when no variable is declared
    add_compile_options( /bigobj )
    add_definitions( /DUNICODE /D_UNICODE ) # it is a new millenium
endif()


# define some folders
get_filename_component(RXCPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)
get_filename_component(RXCPP_DIR "${RXCPP_DIR}" PATH)

MESSAGE( STATUS "RXCPP_DIR: " ${RXCPP_DIR} )

include_directories(SYSTEM ${RXCPP_DIR}/ext/catch/include)
include_directories(${RXCPP_DIR}/Ix/CPP/src ${RXCPP_DIR}/Rx/v2/src)

# define the sources
set(SAMPLE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
add_executable(${SAMPLE_PROJECT} WIN32 ${SAMPLE_SOURCES})
TARGET_LINK_LIBRARIES(${SAMPLE_PROJECT} ${CMAKE_THREAD_LIBS_INIT})

# configure unit tests via CTest
enable_testing()
set(CTEST_CONFIGURATION_TYPE "${JOB_BUILD_CONFIGURATION}")

add_test(NAME RunTests 
     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
     COMMAND ${SAMPLE_PROJECT} ${TEST_ARGS})