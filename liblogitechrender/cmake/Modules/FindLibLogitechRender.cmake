# - Try to find LibLogitechRender
# Once done this will define
#  LIBLOGITECHRENDER_FOUND - System has LibLogitechRender
#  LIBLOGITECHRENDER_INCLUDE_DIRS - The LibLogitechRender include directories
#  LIBLOGITECHRENDER_LIBRARIES - The libraries needed to use LibLogitechRender

find_package(PkgConfig)

pkg_check_modules(PC_LIBLOGITECHRENDER QUIET liblogitechrender-1.0)

find_path(LIBLOGITECHRENDER_INCLUDE_DIR liblogitechrender.h HINTS ${PC_LIBLOGITECHRENDER_INCLUDEDIR} ${PC_LIBLOGITECHRENDER_INCLUDE_DIRS} PATH_SUFFIXES logitools)

find_library(LIBLOGITECHRENDER_LIBRARY NAMES logitechrender HINTS ${PC_LIBLOGITECHRENDER_LIBDIR} ${PC_LIBLOGITECHRENDER_LIBRARY_DIRS})

set(LIBLOGITECHRENDER_LIBRARIES ${LIBLOGITECHRENDER_LIBRARY})
set(LIBLOGITECHRENDER_INCLUDE_DIRS ${LIBLOGITECHRENDER_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibLogitechRender DEFAULT_MSG LIBLOGITECHRENDER_LIBRARY LIBLOGITECHRENDER_INCLUDE_DIR)

mark_as_advanced(LIBLOGITECHRENDER_INCLUDE_DIR LIBLOGITECHRENDER_LIBRARY) 
