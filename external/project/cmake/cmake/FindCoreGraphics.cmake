# - Find CoreGraphics
# Find the native OpenGLES3 includes and libraries
#
#  COREGRAPHICS_INCLUDE_DIR - where to find GLES3/gl3.h, etc.
#  COREGRAPHICS_LIBRARIES   - List of libraries when using OpenGLES3.
#  COREGRAPHICS_FOUND       - True if OpenGLES3 found.

# Downloaded from
# https://sourceforge.net/p/alleg/allegro/ci/5.1/tree/cmake/FindOpenGLES.cmake
# Modified to use the v3 library rather than the v1 library.

find_path(COREGRAPHICS_INCLUDE_DIR CoreGraphics/CGDirectDisplay.h)

find_library(COREGRAPHICS_LIBRARY NAMES CoreGraphics)

# Handle the QUIETLY and REQUIRED arguments and set OPENGLES_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OPENGLES3 DEFAULT_MSG
    COREGRAPHICS_INCLUDE_DIR COREGRAPHICS_LIBRARY)

mark_as_advanced(COREGRAPHICS_INCLUDE_DIR)
mark_as_advanced(COREGRAPHICS_LIBRARY)

