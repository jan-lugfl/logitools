cmake_minimum_required (VERSION 2.6)
project (liblogitech) 

set(PACKAGE_STRING "liblogitech")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
include_directories("${PROJECT_BINARY_DIR}")

add_library(logitech SHARED liblogitech.c)

install(TARGETS logitech LIBRARY DESTINATION lib)
install(FILES liblogitech.h DESTINATION include/logitech)