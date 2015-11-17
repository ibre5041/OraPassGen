cmake_minimum_required (VERSION 3.0.0)
project (DBPASS)

SET (VERSION_MAJOR "0")
SET (VERSION_MINOR "0")
SET (VERSION_PATCH "1")

#Pretty colors
SET(CMAKE_COLOR_MAKEFILE ON)
#Dont force verbose
SET(CMAKE_VERBOSE_MAKEFILE OFF)
#Include current dir
SET(CMAKE_INCLUDE_CURRENT_DIR TRUE)

#Set the custom CMake module directory where our include/lib finders are
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

FIND_PACKAGE(Boost 1.46.0 REQUIRED)
FIND_PACKAGE(OpenSSL)
FIND_PACKAGE(Oracle COMPONENTS CLNTSH)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5PrintSupport REQUIRED)
FIND_PACKAGE(Qt5Sql REQUIRED)
FIND_PACKAGE(Qt5Xml REQUIRED)
set(QT_INCLUDES
  ${Qt5Core_INCLUDE_DIRS}	  
  ${Qt5Gui_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
  ${Qt5Network_INCLUDE_DIRS}
  ${Qt5PrintSupport_INCLUDE_DIRS}
  ${Qt5Sql_INCLUDE_DIRS}
  ${Qt5Xml_INCLUDE_DIRS}
  )
INCLUDE_DIRECTORIES(${QT_INCLUDES})
#SET(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
#SET(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS}   ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")       

IF (CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_CXX_FLAGS_DEBUG   "-O0 -fno-inline -rdynamic")
  SET(CMAKE_C_FLAGS_DEBUG     "-O0 -fno-inline -rdynamic")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O2")
  SET(CMAKE_C_FLAGS_RELEASE   "-O2")
  SET(CMAKE_CXX_FLAGS         "-Wall -std=c++11 -ggdb3")
  SET(CMAKE_C_FLAGS           "-Wall            -ggdb3")
ENDIF (CMAKE_COMPILER_IS_GNUCC)

#Convert our simpler command line option to the CMake style
#None, Debug, Release, .. or custom ones
IF (CMAKE_BUILD_TYPE MATCHES [Dd]ebug)
  message(STATUS "Debug build")
  set(CMAKE_BUILD_TYPE Debug)
  add_definitions(-DDEBUG -DQT_DEBUG)
  set(TORA_DEBUG TRUE)
ELSE()
  message(STATUS "non-debug build")
  set(TORA_DEBUG FALSE)
  add_definitions(-DNDEBUG -DQT_NO_DEBUG)
  add_definitions(-DQT_NO_DEBUG_OUTPUT)
ENDIF()

# Lets disable Wall warnings due the MSVC too much talkative
# output. Wall can slow down compiletion up to hours and
# my boxes can boil eggs too.
# MSVC, MSVC_IDE, MSVC60, MSVC70, MSVC71, MSVC80, CMAKE_COMPILER_2005
IF (MSVC)
  SET (CXX_WARNINGS "")
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ELSE (MSVC)
  SET (CXX_WARNINGS "-Wall")
ENDIF (MSVC)

INCLUDE_DIRECTORIES(
	src/trotl/src
	src/trotl
	src/loki/include
	${ORACLE_INCLUDES}
	${QT_INCLUDES}
	${OPENSSL_INCLUDE_DIR}
	${Boost_INCLUDE_DIRS}
	)

SET(TROTL_DLL_DEFINES    -DLOKI_STATIC -DEXPLICIT_EXPORT -DTROTL_MAKE_DLL)
SET(TROTL_CLIENT_DEFINES -DLOKI_STATIC -DEXPLICIT_EXPORT -DTROTL_DLL)

#ADD_SUBDIRECTORY (extlibs)
ADD_SUBDIRECTORY (src)

