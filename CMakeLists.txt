# CMake file
#
# Adapted from:
# http://docs.opencv.org/2.4/doc/tutorials/introduction/linux_gcc_cmake/linux_gcc_cmake.html
#
cmake_minimum_required(VERSION 2.8)
project( ascii13 )
# require OpenCV packages
find_package( OpenCV REQUIRED )
# link main project
add_executable( ascii13 ascii13.cpp )
target_link_libraries( ascii13 ${OpenCV_LIBS} )
