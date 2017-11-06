cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

PROJECT (PicoVimDemo)

MESSAGE(STATUS " CMakeLists: PicoVim")

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if (CMAKE_BUILD_TYPE MATCHES Coverage)
    set (CMAKE_BUILD_TYPE "Debug")
    set (PROJECT_COVERAGE ON)
endif()

# global needed variables
SET (APPLICATION_NAME "PicoVim")
SET (APPLICATION_VERSION_MAJOR "0")
SET (APPLICATION_VERSION_MINOR "1")
SET (APPLICATION_VERSION_PATCH "0")
SET (APPLICATION_VERSION "${APPLICATION_VERSION_MAJOR}.${APPLICATION_VERSION_MINOR}.${APPLICATION_VERSION_PATCH}")

# Core stuff
SET(PICOVIM_SOURCE "")
SET(PICOVIM_INCLUDE "")

# Third party sources
SET(M3RDPARTY_SOURCE "")
SET(M3RDPARTY_INCLUDE "")

# Set all compiler flags 
INCLUDE(cmake/all.cmake)

# Functions for file copying
INCLUDE(m3rdparty/cmake/copy_files.cmake)

# config_app.h checks
# This makes a config_shared.h file which can be included for system settings
#  Must be after setting up the platforms
CONFIGURE_FILE(${CMAKE_CURRENT_LIST_DIR}/cmake/config_app.h.cmake ${CMAKE_BINARY_DIR}/config_app.h)

INCLUDE(m3rdparty/list.cmake)
INCLUDE(src/list.cmake)

INCLUDE_DIRECTORIES(
    m3rdparty
    ${M3RDPARTY_INCLUDE}
    ${SRC_INCLUDE}
    ${CMAKE_BINARY_DIR}
    .
    ) 

# Create the library - No depenencies on anything else in this file
ADD_LIBRARY(PicoVim STATIC ${PICOVIM_SOURCE} ${PICOVIM_INCLUDE})

# Create the app ***
IF (OPENGL_FOUND OR TARGET_PC)
ADD_EXECUTABLE (${PROJECT_NAME} WIN32 main.cpp ${M3RDPARTY_SOURCE}) # Win32 ignored on non-windows
copy_existing_files(${PROJECT_NAME} m3rdparty/imgui/extra_fonts/ProggyClean.ttf ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration) )
TARGET_LINK_LIBRARIES (${PROJECT_NAME} PicoVim ${PLATFORM_LINKLIBS})
ADD_DEPENDENCIES(${PROJECT_NAME} sdl2)
ENDIF()

INCLUDE(tests/list.cmake)
enable_testing()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGTEST_HAS_TR1_TUPLE=0")
SET (TEST_SOURCES
    ${M3RDPARTY_DIR}/googletest/googletest/src/gtest-all.cc
    ${TEST_SOURCES}
)
ADD_EXECUTABLE (unittests ${TEST_SOURCES} )
ADD_DEPENDENCIES(unittests sdl2)
TARGET_LINK_LIBRARIES (unittests PicoVim ${PLATFORM_LINKLIBS} ${CMAKE_THREAD_LIBS_INIT})
add_test(unittests unittests)
INCLUDE_DIRECTORIES(
    ${M3RDPARTY_DIR}/googletest/googletest/include
    ${M3RDPARTY_DIR}/googletest/googletest
    ${M3RDPARTY_DIR}/googletest/googlemock/include
    ${M3RDPARTY_DIR}/googletest/googlemock
)

SOURCE_GROUP (PicoVim REGULAR_EXPRESSION "src/.*")
