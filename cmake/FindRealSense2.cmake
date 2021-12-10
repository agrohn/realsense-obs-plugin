# - Find libjson source/include folder
# This module finds realsense2 if it is installed and determines where
# the files are. This code sets the following variables:
#
#  REALSENSE2_FOUND             - have realsense2 been found
#  REALSENSE2_INCLUDE_DIR       - path to where rs.h is found
#  REALSENSE2_LIBRARY_DIR        - path to where librealsense2 is found
#  REALSENSE2_LIBRARY           - path to where librealsense2 is found
#
FIND_PATH(REALSENSE2_INCLUDE_DIR librealsense2/rs.h
  HINTS $ENV{REALSENSE_ROOT} 
  PATH_SUFFIXES include
  PATHS ~/Library/Frameworks
  "C:/Program Files (x86)/Intel RealSense SDK 2.0"
  /usr/include
  ${CMAKE_SOURCE_DIR}/deps
  NO_DEFAULT_PATH
  )
find_library(REALSENSE2_LIBRARY
        realsense2
        HINTS $ENV{REALSENSE2_ROOT}
        PATH_SUFFIXES lib64 lib/x64
        PATHS ${CMAKE_SOURCE_DIR}/deps
	     ~/Library/Frameworks
        /usr
        "C:/Program Files (x86)/Intel RealSense SDK 2.0"
	 NO_DEFAULT_PATH
	 
)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RealSense2 DEFAULT_MSG REALSENSE2_LIBRARY)

mark_as_advanced(  REALSENSE2_LIBRARY REALSENSE2_INCLUDE_DIR)

