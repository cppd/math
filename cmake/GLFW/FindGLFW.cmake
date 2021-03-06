#  GLFW_FOUND
#  GLFW_INCLUDE_DIRS
#  GLFW_LIBRARIES

if (GLFW_INCLUDE_DIRS AND GLFW_LIBRARIES)
        set(GLFW_FIND_QUIETLY TRUE)
endif()

find_library(GLFW_LIBRARIES NAMES glfw libglfw glfw3 libglfw3)
find_path(GLFW_MAIN_INCLUDE_DIR NAMES GLFW/glfw3.h)
find_path(GLFW_NATIVE_INCLUDE_DIR NAMES GLFW/glfw3native.h)

if (GLFW_MAIN_INCLUDE_DIR AND GLFW_NATIVE_INCLUDE_DIR)
        set(GLFW_INCLUDE_DIRS "${GLFW_MAIN_INCLUDE_DIR}" "${GLFW_NATIVE_INCLUDE_DIR}")
        list(REMOVE_DUPLICATES GLFW_INCLUDE_DIRS)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLFW DEFAULT_MSG GLFW_INCLUDE_DIRS GLFW_LIBRARIES)
mark_as_advanced(GLFW_INCLUDE_DIRS GLFW_LIBRARIES)
