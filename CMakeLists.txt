CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( pixman-region CXX C )

# default to debug build if unspecified by user
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# build library
FILE (GLOB_RECURSE PIXMAN_SRC "pixman-src/*" "pixman-region/*" )
ADD_LIBRARY ( pixman-region STATIC ${PIXMAN_SRC} )

# build test app if building in debug mode
string( TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower )
if( build_type_lower STREQUAL "debug" )
    FILE (GLOB PIXMANTEST_SRC "test/*" )
    ADD_EXECUTABLE ( pixman-test ${PIXMANTEST_SRC} )
    INCLUDE_DIRECTORIES( "." )
    TARGET_LINK_LIBRARIES( pixman-test pixman-region )
ENDIF(build_type_lower STREQUAL "debug" )

