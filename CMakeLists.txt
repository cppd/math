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

cmake_minimum_required(VERSION 3.30)

##################################################

include("${CMAKE_SOURCE_DIR}/cmake/add_include_files.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/add_source_files.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/check_compilers.cmake")

##################################################

set(vulkan_api_version 0x0103)
set(shader_environment "vulkan1.3")
set(extensions_shaders "comp;frag;geom;rahit;rcall;rchit;rgen;rint;rmiss;vert;tesc;tese")
set(extensions_shaders_include "glsl")
set(extensions_all "h;c;cpp;ui" ${extensions_shaders} ${extensions_shaders_include})
set(qt6_version 6.7)
set(qt6_disable_deprecated_before 0x060700)

##################################################

macro(set_compiler)
        check_compiler_versions(
                GCC_MINIMAL_VERSION 14
                CLANG_MINIMAL_VERSION 19)

        set(CMAKE_C_STANDARD 23)
        set(CMAKE_C_STANDARD_REQUIRED ON)
        set(CMAKE_C_EXTENSIONS OFF)
        set(CMAKE_CXX_STANDARD 23)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        set(CMAKE_CXX_EXTENSIONS OFF)

        # "make VERBOSE=1"
        set(CMAKE_VERBOSE_MAKEFILE TRUE)

        set(CMAKE_C_FLAGS)
        set(CMAKE_CXX_FLAGS)

        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
                if(BUILD_LIB_CPP)
                        set(CMAKE_CXX_FLAGS "-stdlib=libc++")
                endif()
        elseif(BUILD_LIB_CPP)
                message(FATAL_ERROR "BUILD_LIB_CPP and not Clang")
        endif()

        check_compiler_int128()
endmacro()

##################################################

project(create_str C CXX)

set_compiler()

add_source_files(
        EXECUTABLE
        DIRECTORIES "src_str"
        EXTENSIONS ${extensions_all})

##################################################

project(math C CXX)

set_compiler()

set(output_headers_directory "${PROJECT_BINARY_DIR}/output_headers")
file(REMOVE_RECURSE "${output_headers_directory}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_source_files(
        EXECUTABLE
        VULKAN_API_VERSION ${vulkan_api_version}
        DIRECTORIES "src"
        EXTENSIONS ${extensions_all})

add_include_files(
        BINARY_FILES
        CREATE_STR "${PROJECT_BINARY_DIR}/create_str"
        OUTPUT_DIRECTORY "${output_headers_directory}/fonts"
        OUTPUT_EXTENSION "bin"
        SOURCE "${PROJECT_SOURCE_DIR}/font"
        EXTENSIONS "ttf")

function(add_shaders)
        cmake_parse_arguments(PARSE_ARGV 0 ARG "" "SOURCE" "MACROS")
        add_include_files(
                VULKAN_SHADERS
                GLSL_COMPILER Vulkan::glslangValidator
                CREATE_STR "${PROJECT_BINARY_DIR}/create_str"
                OUTPUT_DIRECTORY "${output_headers_directory}/shaders"
                OUTPUT_EXTENSION "spr"
                SOURCE ${ARG_SOURCE}
                MACROS ${ARG_MACROS}
                INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}"
                INCLUDE_EXTENSIONS ${extensions_shaders_include}
                SHADER_ENVIRONMENT ${shader_environment}
                EXTENSIONS ${extensions_shaders})
endfunction()

add_shaders(SOURCE "${PROJECT_SOURCE_DIR}/src/gpu")
add_shaders(SOURCE "${PROJECT_SOURCE_DIR}/src/gpu/renderer" MACROS RAY_TRACING)

# CMake uses -I
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

##################################################

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
if(NOT TARGET Vulkan::glslangValidator)
        message(FATAL_ERROR "glslangValidator not found")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GMP")
find_package(GMP REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${GMP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${GMP_C_LIBRARIES} ${GMP_CXX_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        find_package(X11 REQUIRED COMPONENTS xcb)
        target_link_libraries(${PROJECT_NAME} PRIVATE X11::xcb)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)
target_compile_definitions(
        ${PROJECT_NAME} PRIVATE QT_DISABLE_DEPRECATED_BEFORE=${qt6_disable_deprecated_before})
find_package(Qt6 ${qt6_version} REQUIRED COMPONENTS Widgets)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

find_package(Freetype REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/FFTW")
find_package(FFTW)
if(FFTW_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE FFTW_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${FFTW_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${FFTW_LIBRARIES} ${FFTW_THREAD_LIBRARIES})
endif()

find_package(CUDAToolkit)
if(CUDAToolkit_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE CUDA_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE CUDA::cudart CUDA::cufft)
endif()
