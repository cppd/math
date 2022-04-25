#
# Copyright (C) 2017-2022 Topological Manifold
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

cmake_minimum_required(VERSION 3.23)

##################################################

include("${CMAKE_SOURCE_DIR}/cmake/add_source_files.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/add_include_files.cmake")

##################################################

set(compilers "GNU;Clang")
set(gcc_minimal_version "11")
set(clang_minimal_version "14")

set(shader_environment "vulkan1.2")
set(extensions_shaders "comp;frag;geom;rahit;rcall;rchit;rgen;rint;rmiss;vert;tesc;tese")
set(extensions_shaders_include "glsl")
set(extensions_all "h;c;cpp;ui" ${extensions_shaders} ${extensions_shaders_include})

##################################################

enable_language(C CXX)

if(NOT CMAKE_C_COMPILER_ID)
         message(FATAL_ERROR "Empty C compiler id")
endif()
if(NOT CMAKE_CXX_COMPILER_ID)
         message(FATAL_ERROR "Empty C++ compiler id")
endif()

if(NOT (${CMAKE_C_COMPILER_ID} IN_LIST compilers))
        message(FATAL_ERROR "Not supported C compiler ${CMAKE_C_COMPILER_ID}")
endif()
if(NOT (${CMAKE_CXX_COMPILER_ID} IN_LIST compilers))
        message(FATAL_ERROR "Not supported C++ compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

if((${CMAKE_C_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${gcc_minimal_version}))
        message(FATAL_ERROR "Not supported GCC C compiler version ${CMAKE_C_COMPILER_VERSION}")
endif()
if((${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${gcc_minimal_version}))
        message(FATAL_ERROR "Not supported GCC C++ compiler version ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if((${CMAKE_C_COMPILER_ID} STREQUAL "Clang") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${clang_minimal_version}))
        message(FATAL_ERROR "Not supported Clang C compiler version ${CMAKE_C_COMPILER_VERSION}")
endif()
if((${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${clang_minimal_version}))
        message(FATAL_ERROR "Not supported Clang C++ compiler version ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# after enable_language
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# "make VERBOSE=1"
# after project or enable_language
set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

set(CMAKE_C_FLAGS)
set(CMAKE_CXX_FLAGS)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "-stdlib=libc++")
endif()

##################################################

function(check_int128)
        check_c_source_compiles("int main(){__int128 v;}" C_HAS_INT128)
        if(NOT C_HAS_INT128)
                message(FATAL_ERROR "C doesn't have __int128")
        endif()
        check_cxx_source_compiles("int main(){__int128 v;}" CXX_HAS_INT128)
        if(NOT CXX_HAS_INT128)
                message(FATAL_ERROR "C++ doesn't have __int128")
        endif()
endfunction()

##################################################

project(create_str C CXX)

add_source_files(
        EXECUTABLE
        DIRECTORIES "src_str"
        EXTENSIONS ${extensions_all})

##################################################

project(math C CXX)

set(output_headers_directory "${PROJECT_BINARY_DIR}/output_headers")
file(REMOVE_RECURSE "${output_headers_directory}")

# Qt
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOMOC_MOC_OPTIONS some_options)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

check_int128()

add_source_files(
        EXECUTABLE
        DIRECTORIES "src"
        EXTENSIONS ${extensions_all})

add_include_files(
        BINARY_FILES
        CREATE_STR "${PROJECT_BINARY_DIR}/create_str"
        OUTPUT_DIRECTORY "${output_headers_directory}/fonts"
        OUTPUT_EXTENSION "bin"
        SOURCE "font"
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
                INCLUDE_DIRECTORIES "src/shading"
                INCLUDE_EXTENSIONS ${extensions_shaders_include}
                SHADER_ENVIRONMENT ${shader_environment}
                EXTENSIONS ${extensions_shaders})
endfunction()

add_shaders(SOURCE "src/gpu")
add_shaders(SOURCE "src/gpu/renderer" MACROS RAY_TRACING)

target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

# CMake uses -I
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

##################################################

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
if(NOT TARGET Vulkan::glslangValidator)
        message(FATAL_ERROR "glslangValidator not found")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GMP")
find_package(GMP REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GMP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GMP_C_LIBRARIES} ${GMP_CXX_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets X11Extras)
        target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::X11Extras)
else()
        find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)
        target_link_libraries(${PROJECT_NAME} Qt5::Widgets)
endif()

find_package(Freetype REQUIRED)
target_link_libraries(${PROJECT_NAME} Freetype::Freetype)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/FFTW")
find_package(FFTW)
if(FFTW_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE FFTW_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${FFTW_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES} ${FFTW_THREAD_LIBRARIES})
endif()

find_package(CUDAToolkit)
if(CUDAToolkit_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE CUDA_FOUND)
        target_link_libraries(${PROJECT_NAME} CUDA::cudart CUDA::cufft)
endif()
