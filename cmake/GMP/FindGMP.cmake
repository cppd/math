#  GMP_FOUND
#  GMP_INCLUDE_DIRS
#  GMP_C_LIBRARIES
#  GMP_CXX_LIBRARIES

if (GMP_INCLUDE_DIRS AND GMP_C_LIBRARIES AND GMP_CXX_LIBRARIES)
        set(GMP_FIND_QUIETLY TRUE)
endif()

find_library(GMP_C_LIBRARIES NAMES gmp libgmp)
find_library(GMP_CXX_LIBRARIES NAMES gmpxx libgmpxx)
find_path(GMP_C_INCLUDES NAMES gmp.h)
find_path(GMP_CXX_INCLUDES NAMES gmpxx.h)

if (GMP_C_INCLUDES AND GMP_CXX_INCLUDES)
        set(GMP_INCLUDE_DIRS "${GMP_C_INCLUDES}" "${GMP_CXX_INCLUDES}")
        list(REMOVE_DUPLICATES GMP_INCLUDE_DIRS)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_INCLUDE_DIRS GMP_C_LIBRARIES GMP_CXX_LIBRARIES)
mark_as_advanced(GMP_INCLUDE_DIRS GMP_C_LIBRARIES GMP_CXX_LIBRARIES)