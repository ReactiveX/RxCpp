
# Yes, INIT_BIICODE_BLOCKS() is no longer necessary
ADD_BIICODE_TARGETS()

# You can safely delete lines from here...

###############################################################################
#      REFERENCE                                                              #
###############################################################################
#
# This CMakeLists.txt file helps defining your block building and compiling
# To learn more about the CMake use with biicode, visit http://docs.biicode.com/c++.html
#
# ----------------------------------------------------
# NEW FEATURE! Include cmake files from remote blocks:
# -----------------------------------------------------
# Now you can handle cmake dependencies alike you do with c/c++:
#
#     INCLUDE(user/block/myrecipe) # include myrecipe.cmake from remote user/block
#
# > EXAMPLE: Include our recipes and activate C++11 in your block (http://www.biicode.com/biicode/cmake)
#
#     INCLUDE(biicode/cmake/tools) # Include tools.cmake file from "cmake" block from the "biicode" user
#     ACTIVATE_CPP11(INTERFACE ${BII_BLOCK_TARGET})
#
#     Remember to run "bii find" to download out cmake tools file
#
# ---------------------
# INIT_BIICODE_BLOCK()
# ---------------------
# This function creates several helper variables as ${BII_BLOCK_NAME} and ${BII_BLOCK_USER}
# Also it loads variables from the cmake/bii_user_block_vars.cmake
#     ${BII_LIB_SRC}          File list to create the library
#     ${BII_LIB_TYPE}         Empty (default, STATIC most casess) STATIC or SHARED
#     ${BII_LIB_DEPS}         Dependencies to other libraries (user2_block2, user3_blockX)
#     ${BII_LIB_SYSTEM_HEADERS}  System linking requirements as windows.h, pthread.h, etc
#
# You can use or modify them here, for example, to add or remove files from targets based on OS
# Or use typical cmake configurations done BEFORE defining targets. Examples:
#     ADD_DEFINITIONS(-DFOO)
#     FIND_PACKAGE(OpenGL QUIET)
#     You can add INCLUDE_DIRECTORIES here too
#
# ---------------------
# ADD_BIICODE_TARGETS()
# ---------------------
#
# This function creates the following variables:
#     ${BII_BLOCK_TARGET}  Interface (no files) target for convenient configuration of all
#                          targets in this block, as the rest of targets always depend on it
#                          has name in the form "user_block_interface"
#     ${BII_LIB_TARGET}  Target library name, usually in the form "user_block". May not exist
#                        if BII_LIB_SRC is empty
#     ${BII_BLOCK_TARGETS} List of all targets defined in this block
#     ${BII_BLOCK_EXES} List of executables targets defined in this block
#     ${BII_exe_name_TARGET}: Executable target (e.g. ${BII_main_TARGET}. You can also use
#                            directly the name of the executable target (e.g. user_block_main)
#
# > EXAMPLE: Add include directories to all targets of this block
#
#    TARGET_INCLUDE_DIRECTORIES(${BII_BLOCK_TARGET} INTERFACE myincludedir)
#
#    You can add private include directories to the Lib (if existing)
#
# > EXAMPLE: Link with pthread:
#
#    TARGET_LINK_LIBRARIES(${BII_BLOCK_TARGET} INTERFACE pthread)
#        or link against library:
#    TARGET_LINK_LIBRARIES(${BII_LIB_TARGET} PUBLIC pthread)
#        or directly use the library target name:
#    TARGET_LINK_LIBRARIES(user_block PUBLIC pthread)
#
#    NOTE:  This can be also done adding pthread to ${BII_LIB_DEPS}
#            BEFORE calling ADD_BIICODE_TARGETS()
#
# > EXAMPLE: how to activate C++11
#
#    IF(APPLE)
#         TARGET_COMPILE_OPTIONS(${BII_BLOCK_TARGET} INTERFACE "-std=c++11 -stdlib=libc++")
#    ELSEIF (WIN32 OR UNIX)
#         TARGET_COMPILE_OPTIONS(${BII_BLOCK_TARGET} INTERFACE "-std=c++11")
#    ENDIF(APPLE)
#
# > EXAMPLE: Set properties to target
#
#    SET_TARGET_PROPERTIES(${BII_BLOCK_TARGET} PROPERTIES COMPILE_DEFINITIONS "IOV_MAX=255")
#