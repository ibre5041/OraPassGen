# vim: set ts=2 shiftwidth=2 expandtab:
# - Find GMP/MPIR libraries and headers
# This module defines the following variables:
#
# MPIR_FOUND         - true if MPIR/MPIR was found
# MPIR_INCLUDE_DIRS  - include search path
# MPIR_LIBARIES      - libraries to link with
# MPIR_LIBARY_DLL    - library DLL to install. Only available on WIN32.
# MPIR_LIBRARIES_DIR - the directory the library we link with is found in.

find_path(MPIR_INCLUDE_DIRS NAMES mpir.h
  PATHS "$ENV{PROGRAMFILES}/mpir/include"
  DOC "The gmp include directory"
)
find_path(MPIRXX_INCLUDE_DIRS NAMES mpirxx.h
  PATHS "$ENV{PROGRAMFILES}/mpir/include"
  DOC "The gmp include directory"
)

if(WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND MSVC)
    set(MPIR_LIB "mpird")
  else()
    set(MPIR_LIB "mpir")
  endif()

  find_library(MPIR_LIBRARIES NAMES ${MPIR_LIB}
    PATHS "$ENV{PROGRAMFILES}/mpir/lib"
    DOC "The MPIR library"
    )
  find_library(MPIR_LIBRARY_DLL NAMES ${MPIR_LIB}
    PATHS "$ENV{PROGRAMFILES}/mpir/bin"
    DOC "The MPIR library DLL"
    )
else(WIN32)
  find_library(MPIR_LIBRARIES NAMES mpir
    PATHS "$ENV{PROGRAMFILES}/mpir/lib" "/usr/lib"
    DOC "The MPIR library"
    )
  find_library(MPIR_LIBRARIES_STATIC NAMES libmpir.a
    PATHS "$ENV{PROGRAMFILES}/mpir/lib" "/usr/lib"
    DOC "The MPIR library"
    )
  find_library(MPIRXX_LIBRARIES_STATIC NAMES libmpirxx.a
    PATHS "$ENV{PROGRAMFILES}/mpir/lib" "/usr/lib"
    DOC "The MPIR library"
    )
	      
endif(WIN32)

get_filename_component(MPIR_LIBRARIES_DIR "${MPIR_LIBRARIES}" PATH)

# handle the QUIET and REQUIRED arguments and set MPIR_FOUND to TRUE if
# all listed variables are true
include(FindPackageHandleStandardArgs)
if(WIN32)
  find_package_handle_standard_args(MPIR DEFAULT_MSG MPIR_LIBRARIES MPIR_LIBRARY_DLL MPIR_INCLUDE_DIRS MPIR_LIBRARIES MPIRXX_LIBRARIES)
else()
  find_package_handle_standard_args(MPIR DEFAULT_MSG MPIR_LIBRARIES MPIR_INCLUDE_DIRS MPIR_LIBRARIES_STATIC MPIRXX_LIBRARIES_STATIC)
endif()
