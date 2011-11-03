cmake_minimum_required (VERSION 2.6)
project (liblogitechrender) 

set(PACKAGE_STRING "liblogitechrender")

set(G15FONT_DIR "${CMAKE_INSTALL_PREFIX}/share/liblogitechrender/fonts")

find_package(Freetype)
if(FREETYPE_FOUND)
  set(TTF_SUPPORT 1)
  include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")
configure_file("${PROJECT_SOURCE_DIR}/liblogitechrender.pc.in" "${PROJECT_BINARY_DIR}/liblogitechrender.pc" @ONLY)

include_directories("${PROJECT_BINARY_DIR}")

add_library(logitechrender SHARED src/pixel.c src/screen.c src/text.c)
add_executable(logitechfontconvert src/logitechfontconvert.c)

if(FREETYPE_FOUND)
  target_link_libraries(logitechrender ${FREETYPE_LIBRARIES} m)
  target_link_libraries(logitechfontconvert ${FREETYPE_LIBRARIES} logitechrender)
endif()

install(TARGETS logitechfontconvert logitechrender RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(FILES "${PROJECT_BINARY_DIR}/liblogitechrender.pc" DESTINATION lib/pkgconfig)
install(FILES src/liblogitechrender.h DESTINATION include/logitools)
install(DIRECTORY fonts DESTINATION share/liblogitechrender)
install(FILES cmake/Modules/FindLibLogitechRender.cmake DESTINATION share/cmake/Modules)