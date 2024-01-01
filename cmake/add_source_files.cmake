#
# Copyright (C) 2017-2024 Topological Manifold
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include("${CMAKE_SOURCE_DIR}/cmake/set_compiler_warnings.cmake")

function(add_source_files TARGET_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG
                ""
                "VULKAN_API_VERSION"
                "EXTENSIONS;DIRECTORIES")

        if(NOT ARG_EXTENSIONS)
                message(FATAL_ERROR "No extensions")
        endif()
        if(NOT ARG_DIRECTORIES)
                message(FATAL_ERROR "No directories")
        endif()
        if(ARG_UNPARSED_ARGUMENTS)
                message(FATAL_ERROR "Unparsed arguments ${ARG_UNPARSED_ARGUMENTS}")
        endif()

        #

        if("${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
                message(FATAL_ERROR "In-source build")
        endif()

        #

        foreach(dir ${ARG_DIRECTORIES})
                foreach(ext ${ARG_EXTENSIONS})
                        list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir}/*.${ext}")
                endforeach()
        endforeach()
        file(GLOB_RECURSE all_files LIST_DIRECTORIES false ${all_globbing_expressions})

        if("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
                add_executable(${PROJECT_NAME} ${all_files})
        elseif("${TARGET_TYPE}" STREQUAL "LIBRARY")
                add_library(${PROJECT_NAME} SHARED ${all_files})
        else()
                message(FATAL_ERROR "Unknown target type ${TARGET_TYPE}")
        endif()

        target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)

        if(ARG_VULKAN_API_VERSION)
                target_compile_definitions(
                        ${PROJECT_NAME} PRIVATE BUILD_VULKAN_API_VERSION=${ARG_VULKAN_API_VERSION})
        endif()

        target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
        if(BUILD_DEBUG AND BUILD_RELEASE)
                message(FATAL_ERROR "BUILD_DEBUG and BUILD_RELEASE")
        elseif(BUILD_DEBUG)
                target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_DEBUG)
                target_compile_options(${PROJECT_NAME} PRIVATE -Og -ggdb)
        elseif(BUILD_RELEASE)
                target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_RELEASE)
                target_compile_options(${PROJECT_NAME} PRIVATE -O3)
        else()
                target_compile_options(${PROJECT_NAME} PRIVATE -O3)
        endif()

        if(BUILD_LIB_CPP)
                target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_LIB_CPP)
        endif()

        target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -fno-rtti -fstrict-enums >)
        # target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>: -mllvm -inline-threshold=1000 >)

        set_compiler_warnings("${all_files}")

endfunction()
