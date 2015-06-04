FIND_PACKAGE(Threads)

# define some compiler settings

MESSAGE( STATUS "CMAKE_CXX_COMPILER_ID: " ${CMAKE_CXX_COMPILER_ID} )

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    MESSAGE( STATUS "using clang settings" )
    set(RX_COMPILE_OPTIONS
        -Wall -Wextra -Werror -Wunused
        -stdlib=libc++
        -ftemplate-depth=1024
        )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    MESSAGE( STATUS "using gnu settings" )
    set(RX_COMPILE_OPTIONS
        -Wall -Wextra -Werror -Wunused
        )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    MESSAGE( STATUS "using msvc settings" )
    set(RX_COMPILE_OPTIONS
        /W4 /WX
        /wd4503 # truncated symbol
        /wd4702 # unreachable code
        /bigobj
        /DUNICODE /D_UNICODE # it is a new millenium
        )
endif()

set(RX_COMPILE_FEATURES
    cxx_auto_type
    cxx_nullptr
    cxx_decltype
    cxx_lambdas
    cxx_range_for
    cxx_right_angle_brackets
    cxx_rvalue_references
    cxx_static_assert
    cxx_trailing_return_types
    cxx_alias_templates
    cxx_variadic_templates
    cxx_template_template_parameters
    )

set(IX_SRC_DIR ${RXCPP_DIR}/Ix/CPP/src)
set(RX_SRC_DIR ${RXCPP_DIR}/Rx/v2/src)
set(RX_CATCH_DIR ${RXCPP_DIR}/ext/catch/include)
