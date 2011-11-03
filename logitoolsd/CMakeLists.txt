cmake_minimum_required (VERSION 2.6)
project (logitoolsd) 

find_package(LibLogitech)
find_package(LibLogitechRender)

set(PACKAGE_STRING "logitoolsd")
set(PLUGINDIR "${CMAKE_INSTALL_PREFIX}/share/logitechd/plugins")
set(DATADIR "${CMAKE_INSTALL_PREFIX}/share/logitechd")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
#configure_file("${PROJECT_SOURCE_DIR}/liblogitechrender.pc.in" "${PROJECT_BINARY_DIR}/liblogitechrender.pc" @ONLY)

include_directories(${LIBLOGITECH_INCLUDE_DIRS} ${LIBLOGITECHRENDER_INCLUDE_DIRS} "${PROJECT_BINARY_DIR}")

add_executable(logitoolsd logitoolsd/logitools_plugins.c logitoolsd/linked_lists.c logitoolsd/utility_funcs.c logitoolsd/main.c)
target_link_libraries(logitoolsd ${LIBLOGITECH_LIBRARIES} ${LIBLOGITECHRENDER_LIBRARIES} usb dl)
set_target_properties(logitoolsd PROPERTIES COMPILE_FLAGS "-DG15DAEMON_BUILD")
add_library(logitoolsdl SHARED liblogitoolsdl/logitoolsdl_net.c)

add_library(g15_plugin_clock SHARED plugins/g15_plugin_clock.c)

install(TARGETS logitoolsd logitoolsdl RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(TARGETS g15_plugin_clock LIBRARY DESTINATION share/logitechd/plugins)
#install(FILES "${PROJECT_BINARY_DIR}/liblogitechrender.pc" DESTINATION lib/pkgconfig)
install(FILES liblogitoolsdl/logitoolsdl.h DESTINATION include/logitools)
#install(FILES cmake/Modules/FindLibLogitechRender.cmake DESTINATION share/cmake/Modules)
install(FILES Documentation/logitoolsd.1 DESTINATION man/man1)
install(FILES Documentation/logitoolsdl.3 DESTINATION man/man3)
install(FILES images/g15logo2.wbmp DESTINATION share/logitechd/splash)