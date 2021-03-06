#
# Copyright (C) 2017-2021 Topological Manifold
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

cmake_minimum_required(VERSION 3.18)

##################################################

include("${CMAKE_SOURCE_DIR}/cmake/cmake_compiler_warnings.cmake")

##################################################

set(compilers "GNU;Clang")
set(gcc_minimal_version "10")
set(clang_minimal_version "11")

set(extensions_shaders "comp;frag;geom;vert;tesc;tese")
set(extensions_shaders_include "glsl")
set(extensions_all "h;c;cpp;ui" ${extensions_shaders} ${extensions_shaders_include})
set(extensions_binary "ttf")

##################################################

enable_language(C CXX)

if (NOT CMAKE_C_COMPILER_ID)
         message(FATAL_ERROR "Empty C compiler id")
endif()
if (NOT CMAKE_CXX_COMPILER_ID)
         message(FATAL_ERROR "Empty C++ compiler id")
endif()

if (NOT (${CMAKE_C_COMPILER_ID} IN_LIST compilers))
        message(FATAL_ERROR "Not supported C compiler ${CMAKE_C_COMPILER_ID}")
endif()
if (NOT (${CMAKE_CXX_COMPILER_ID} IN_LIST compilers))
        message(FATAL_ERROR "Not supported C++ compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

if ((${CMAKE_C_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${gcc_minimal_version}))
        message(FATAL_ERROR "Not supported GCC C compiler version ${CMAKE_C_COMPILER_VERSION}")
endif()
if ((${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${gcc_minimal_version}))
        message(FATAL_ERROR "Not supported GCC C++ compiler version ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if ((${CMAKE_C_COMPILER_ID} STREQUAL "Clang") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${clang_minimal_version}))
        message(FATAL_ERROR "Not supported Clang C compiler version ${CMAKE_C_COMPILER_VERSION}")
endif()
if ((${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${clang_minimal_version}))
        message(FATAL_ERROR "Not supported Clang C++ compiler version ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# При наличии enable_language(C CXX) можно указать стандарт для всех проектов
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Чтобы было как "make VERBOSE=1"
# Добавлять после команды project или enable_language
set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

set(CMAKE_C_FLAGS)
set(CMAKE_CXX_FLAGS)
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "-stdlib=libc++")
endif()

##################################################

function(AddSourceFiles TARGET_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "EXTENSIONS;DIRECTORIES")
        if (NOT ARG_EXTENSIONS)
                message(FATAL_ERROR "No extensions")
        endif()
        if (NOT ARG_DIRECTORIES)
                message(FATAL_ERROR "No directories")
        endif()
        if (ARG_UNPARSED_ARGUMENTS)
                message(FATAL_ERROR "Unparsed arguments ${ARG_UNPARSED_ARGUMENTS}")
        endif()

        #

        if ("${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
                message(FATAL_ERROR "In-source build")
        endif()

        #

        foreach(dir ${ARG_DIRECTORIES})
                foreach(ext ${ARG_EXTENSIONS})
                        list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir}/*.${ext}")
                endforeach()
        endforeach()
        file(GLOB_RECURSE all_files LIST_DIRECTORIES false ${all_globbing_expressions})

        if ("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
                add_executable(${PROJECT_NAME} ${all_files})
        elseif ("${TARGET_TYPE}" STREQUAL "LIBRARY")
                add_library(${PROJECT_NAME} SHARED ${all_files})
        else()
                message(FATAL_ERROR "Unknown target type ${TARGET_TYPE}")
        endif()

        target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)

        target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
        if (DEBUG_BUILD)
                target_compile_options(${PROJECT_NAME} PRIVATE -Og -ggdb)
        else()
                target_compile_options(${PROJECT_NAME} PRIVATE -O3)
        endif()
        target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -fno-rtti -fstrict-enums >)
        target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>: -mllvm -inline-threshold=10000 >)

        SetCompilerWarnings("${all_files}")

endfunction()

function(AddIncludeFiles FILE_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG
                ""
                "GLSLANG_VALIDATOR;HEADER_FILE;OUTPUT_EXTENSION;OUTPUT_DIRECTORY"
                "EXTENSIONS;EXTENSIONS_INCLUDE;SOURCE;MACROS")
        if (NOT ARG_OUTPUT_EXTENSION)
                message(FATAL_ERROR "OUTPUT_EXTENSION is not specified")
        endif()
        if (NOT ARG_SOURCE)
                message(FATAL_ERROR "SOURCE is not specified")
        endif()
        if (NOT ARG_OUTPUT_DIRECTORY)
                message(FATAL_ERROR "OUTPUT_DIRECTORY is not specified")
        endif()
        if (ARG_UNPARSED_ARGUMENTS)
                message(FATAL_ERROR "Unparsed arguments ${ARG_UNPARSED_ARGUMENTS}")
        endif()

        #

        unset(BINARY_FILES)
        unset(VULKAN_SHADERS)

        if ("${FILE_TYPE}" STREQUAL "BINARY_FILES")
                if (ARG_HEADER_FILE)
                        message(FATAL_ERROR "Binary file header is not supported")
                endif()
                if (ARG_MACROS)
                        message(FATAL_ERROR "Binary file macros are not supported")
                endif()
                if (ARG_EXTENSIONS_INCLUDE)
                        message(FATAL_ERROR "Binary file include extensions are not supported")
                endif()
                set(BINARY_FILES TRUE)
                set(custom_target_prefix "custom_target_binary_files")
        elseif ("${FILE_TYPE}" STREQUAL "VULKAN_SHADERS")
                if (NOT ARG_GLSLANG_VALIDATOR)
                        message(FATAL_ERROR "GLSLANG_VALIDATOR is not specified")
                endif()
                set(VULKAN_SHADERS TRUE)
                set(custom_target_prefix "custom_target_vulkan_shaders")
        else()
                message(FATAL_ERROR "Unknown file type ${FILE_TYPE}")
        endif()

        #

        file(MAKE_DIRECTORY "${ARG_OUTPUT_DIRECTORY}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${ARG_OUTPUT_DIRECTORY}")

        #

        foreach(source ${ARG_SOURCE})

                set(source_path "${PROJECT_SOURCE_DIR}/${source}")
                if (IS_DIRECTORY "${source_path}")
                        if (NOT ARG_EXTENSIONS)
                                message(FATAL_ERROR "No extensions")
                        endif()
                        unset(globbing_expressions)
                        foreach(ext ${ARG_EXTENSIONS})
                                list(APPEND globbing_expressions "${source_path}/*.${ext}")
                        endforeach()
                        file(GLOB_RECURSE files LIST_DIRECTORIES false ${globbing_expressions})
                else()
                        set(files "${source_path}")
                endif()

                foreach(long_file_name ${files})

                        get_filename_component(file_name "${long_file_name}" NAME)

                        if (VULKAN_SHADERS)
                                foreach(macro ${ARG_MACROS})
                                        string(TOLOWER "_${macro}" macro_lower)
                                        string(APPEND file_name "${macro_lower}")
                                endforeach()
                        endif()

                        set(target_name "${custom_target_prefix}_${file_name}")
                        set(output_file "${ARG_OUTPUT_DIRECTORY}/${file_name}.${ARG_OUTPUT_EXTENSION}")

                        add_custom_target(${target_name} DEPENDS "${output_file}")
                        add_dependencies(${target_name} create_str)
                        add_dependencies(${PROJECT_NAME} ${target_name})

                        if (BINARY_FILES)

                                add_custom_command(
                                        OUTPUT
                                                "${output_file}"
                                        COMMAND
                                                "${PROJECT_BINARY_DIR}/create_str"
                                                "bin" "${long_file_name}" "${output_file}"
                                        DEPENDS
                                                "${long_file_name}"
                                                "${PROJECT_BINARY_DIR}/create_str"
                                        WORKING_DIRECTORY
                                                "${PROJECT_BINARY_DIR}"
                                        VERBATIM
                                )

                        elseif (VULKAN_SHADERS)

                                unset(include_directory)
                                unset(include_files)
                                if (ARG_EXTENSIONS_INCLUDE)
                                        get_filename_component(include_directory "${long_file_name}" DIRECTORY)
                                        unset(globbing_expressions_include)
                                        foreach(ext ${ARG_EXTENSIONS_INCLUDE})
                                                list(APPEND globbing_expressions_include "${include_directory}/*.${ext}")
                                        endforeach()
                                        file(GLOB_RECURSE include_files LIST_DIRECTORIES false ${globbing_expressions_include})
                                endif()

                                unset(header)
                                if (ARG_HEADER_FILE)
                                        set(header "${PROJECT_SOURCE_DIR}/${ARG_HEADER_FILE}")
                                endif()

                                unset(macro_list)
                                foreach(macro ${ARG_MACROS})
                                        list(APPEND macro_list "-D${macro}")
                                endforeach()

                                get_filename_component(file_ext "${long_file_name}" EXT)
                                string(RANDOM LENGTH 10 random_string_glsl)
                                set(output_glsl "${ARG_OUTPUT_DIRECTORY}/${file_name}_${random_string_glsl}${file_ext}")

                                string(RANDOM LENGTH 10 random_string_sprv)
                                set(output_sprv "${ARG_OUTPUT_DIRECTORY}/${file_name}_${random_string_sprv}.sprv")

                                add_custom_command(
                                        OUTPUT
                                                "${output_file}"
                                        COMMAND
                                                "${PROJECT_BINARY_DIR}/create_str"
                                                "cat" ${header} "${long_file_name}" "${output_glsl}"
                                        COMMAND
                                                "${ARG_GLSLANG_VALIDATOR}"
                                                "-I${include_directory}"
                                                "-V"
                                                ${macro_list}
                                                "${output_glsl}"
                                                "-o" "${output_sprv}"
                                        COMMAND
                                                "${PROJECT_BINARY_DIR}/create_str"
                                                "spr" "${output_sprv}" "${output_file}"
                                        DEPENDS
                                                "${header}"
                                                "${long_file_name}"
                                                "${PROJECT_BINARY_DIR}/create_str"
                                                ${include_files}
                                        WORKING_DIRECTORY
                                                "${PROJECT_BINARY_DIR}"
                                        VERBATIM
                                )

                        endif()

                endforeach()

        endforeach()

endfunction()

function(Check_C_Has_Int128)
        check_c_source_compiles("int main(void){__int128 v;}" C_HAS_INT128)
        if (NOT C_HAS_INT128)
                message(FATAL_ERROR "C doesn't have __int128")
        endif()
endfunction()

function(Check_CXX_Has_Int128)
        check_cxx_source_compiles("int main(){__int128 v;}" CXX_HAS_INT128)
        if (NOT CXX_HAS_INT128)
                message(FATAL_ERROR "C++ doesn't have __int128")
        endif()
endfunction()

##################################################

project(create_str C CXX)

AddSourceFiles(EXECUTABLE DIRECTORIES "src_str" EXTENSIONS ${extensions_all})

##################################################

project(math C CXX)

find_program(GLSLANG_VALIDATOR "glslangValidator" REQUIRED)

set(output_headers_directory "${PROJECT_BINARY_DIR}/output_headers")
file(REMOVE_RECURSE "${output_headers_directory}")

# Qt
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOMOC_MOC_OPTIONS some_options)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

Check_C_Has_Int128()
Check_CXX_Has_Int128()

AddSourceFiles(EXECUTABLE
                        DIRECTORIES "src"
                        EXTENSIONS ${extensions_all})

AddIncludeFiles(BINARY_FILES
                        OUTPUT_DIRECTORY "${output_headers_directory}/fonts"
                        OUTPUT_EXTENSION "bin"
                        SOURCE "font"
                        EXTENSIONS ${extensions_binary})

AddIncludeFiles(VULKAN_SHADERS
                        OUTPUT_DIRECTORY "${output_headers_directory}/shaders"
                        GLSLANG_VALIDATOR "${GLSLANG_VALIDATOR}"
                        OUTPUT_EXTENSION "spr"
                        SOURCE
                                "src/gpu/convex_hull/code"
                                "src/gpu/dft/code"
                                "src/gpu/optical_flow/code"
                                "src/gpu/pencil_sketch/code"
                                "src/gpu/renderer/code"
                                "src/gpu/text_writer/code"
                        EXTENSIONS ${extensions_shaders}
                        EXTENSIONS_INCLUDE ${extensions_shaders_include})

target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

# CMake при использовании target_include_directories вставляет -I
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

##################################################

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GMP")
find_package(GMP REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GMP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GMP_C_LIBRARIES} ${GMP_CXX_LIBRARIES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
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
if (FFTW_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE FFTW_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${FFTW_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES} ${FFTW_THREAD_LIBRARIES})
endif()

find_package(CUDAToolkit)
if (CUDAToolkit_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE CUDA_FOUND)
        target_link_libraries(${PROJECT_NAME} CUDA::cudart CUDA::cufft)
endif()
