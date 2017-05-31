#  GLM_FOUND
#  GLM_INCLUDE_DIRS

if (GLM_INCLUDE_DIRS)
        set(GLM_FIND_QUIETLY TRUE)
endif()

find_path(GLM_INCLUDE_DIRS NAMES glm/glm.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLM DEFAULT_MSG GLM_INCLUDE_DIRS)
mark_as_advanced(GLM_INCLUDE_DIRS)
