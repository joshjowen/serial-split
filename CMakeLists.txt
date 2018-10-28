cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(serial_split)
add_executable (serial_split serial_split.cpp)

#install includes
install(TARGETS serial_split
        RUNTIME DESTINATION bin)

