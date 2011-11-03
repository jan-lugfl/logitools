# - Try to find LibLogitech
# Once done this will define
#  LIBLOGITECH_FOUND - System has LibLogitech
#  LIBLOGITECH_INCLUDE_DIRS - The LibLogitech include directories
#  LIBLOGITECH_LIBRARIES - The libraries needed to use LibLogitech

find_package(PkgConfig)

pkg_check_modules(PC_LIBLOGITECH QUIET liblogitech-1.0)

find_path(LIBLOGITECH_INCLUDE_DIR liblogitech.h HINTS ${PC_LIBLOGITECH_INCLUDEDIR} ${PC_LIBLOGITECH_INCLUDE_DIRS} PATH_SUFFIXES logitools)

find_library(LIBLOGITECH_LIBRARY NAMES logitech HINTS ${PC_LIBLOGITECH_LIBDIR} ${PC_LIBLOGITECH_LIBRARY_DIRS})

set(LIBLOGITECH_LIBRARIES ${LIBLOGITECH_LIBRARY})
set(LIBLOGITECH_INCLUDE_DIRS ${LIBLOGITECH_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibLogitech DEFAULT_MSG LIBLOGITECH_LIBRARY LIBLOGITECH_INCLUDE_DIR)

mark_as_advanced(LIBLOGITECH_INCLUDE_DIR LIBLOGITECH_LIBRARY) 
