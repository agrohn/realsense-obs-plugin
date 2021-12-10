cmake_minimum_required(VERSION 3.14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
project(realsense-d400-plugin)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

FIND_PACKAGE( RealSense2 REQUIRED)
FIND_PACKAGE( LibObs REQUIRED )

set(realsense-d400-plugin_SOURCES
  realsense-d400-plugin.cpp
  realsense-d400-source.cpp
	obs_frame_processor.cpp
  )

include_directories( realsense-d400-plugin ${LIBOBS_INCLUDE_DIRS} ${REALSENSE2_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
set( LIBS ${LIBOBS_LIBRARIES} ${REALSENSE2_LIBRARY})
add_library(realsense-d400-plugin MODULE
      ${realsense-d400-plugin_SOURCES})

target_link_libraries(realsense-d400-plugin ${LIBS})
install( TARGETS realsense-d400-plugin  LIBRARY
  DESTINATION  ${LIBOBS_PLUGIN_DESTINATION})
install( TARGETS realsense-d400-plugin  LIBRARY
  DESTINATION ${LIBOBS_PLUGIN_DESTINATION})
install( DIRECTORY "${CMAKE_SOURCE_DIR}/data/locale"
  DESTINATION "${LIBOBS_PLUGIN_DATA_DESTINATION}/realsense-d400")

