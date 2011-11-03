# - Try to find LibLogiToolsDL
# Once done this will define
#  LIBLOGITOOLSDL_FOUND - System has LibLogiToolsDL
#  LIBLOGITOOLSDL_INCLUDE_DIRS - The LibLogiToolsDL include directories
#  LIBLOGITOOLSDL_LIBRARIES - The libraries needed to use LibLogiToolsDL

find_package(PkgConfig)

pkg_check_modules(PC_LIBLOGITOOLSDL QUIET liblogitoolsdl-1.0)

find_path(LIBLOGITOOLSDL_INCLUDE_DIR logitoolsdl.h HINTS ${PC_LIBLOGITOOLSDL_INCLUDEDIR} ${PC_LIBLOGITOOLSDL_INCLUDE_DIRS} PATH_SUFFIXES logitools)

find_library(LIBLOGITOOLSDL_LIBRARY NAMES logitoolsdl HINTS ${PC_LIBLOGITOOLSDL_LIBDIR} ${PC_LIBLOGITOOLSDL_LIBRARY_DIRS})

set(LIBLOGITOOLSDL_LIBRARIES ${LIBLOGITOOLSDL_LIBRARY})
set(LIBLOGITOOLSDL_INCLUDE_DIRS ${LIBLOGITOOLSDL_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibLogiToolsDL DEFAULT_MSG LIBLOGITOOLSDL_LIBRARY LIBLOGITOOLSDL_INCLUDE_DIR)

mark_as_advanced(LIBLOGITOOLSDL_INCLUDE_DIR LIBLOGITOOLSDL_LIBRARY) 
