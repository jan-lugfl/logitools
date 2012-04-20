cmake_minimum_required (VERSION 2.8)
project (logitools-macro)

find_package(X11)
find_package(LibLogitech)
find_package(LibLogiToolsDL)
find_package(LibLogitechRender)

set(PACKAGE_STRING "logitools-macro")
set(DATADIR "${CMAKE_INSTALL_PREFIX}/share/logitechd")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
include_directories(${X11_INCLUDE_DIR} ${LIBLOGITECH_INCLUDE_DIRS} ${LIBLOGITOOLSDL_INCLUDE_DIRS} ${LIBLOGITECHRENDER_INCLUDE_DIRS} "${PROJECT_BINARY_DIR}")

add_executable(logitools-macro fileHandling.c logitools-macro.c)

target_link_libraries(logitools-macro ${X11_LIBRARIES} ${LIBLOGITECH_LIBRARIES} ${LIBLOGITOOLSDL_LIBRARIES} ${LIBLOGITECHRENDER_LIBRARIES} usb pthread)

install(TARGETS logitools-macro RUNTIME DESTINATION bin)
