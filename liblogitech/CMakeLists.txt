cmake_minimum_required (VERSION 2.6)
project (liblogitech) 

set(PACKAGE_STRING "liblogitech")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
configure_file("${PROJECT_SOURCE_DIR}/liblogitech.pc.in" "${PROJECT_BINARY_DIR}/liblogitech.pc" @ONLY)

include_directories("${PROJECT_BINARY_DIR}")

add_library(logitech SHARED liblogitech.c)

install(TARGETS logitech LIBRARY DESTINATION lib)
install(FILES "${PROJECT_BINARY_DIR}/liblogitech.pc" DESTINATION lib/pkgconfig)
install(FILES liblogitech.h DESTINATION include/logitools)
install(FILES cmake/Modules/FindLibLogitech.cmake DESTINATION share/cmake/Modules)