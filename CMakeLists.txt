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

cmake_minimum_required(VERSION 3.22)

##################################################

include("${CMAKE_SOURCE_DIR}/cmake/cmake_compiler_warnings.cmake")

##################################################

set(compilers "GNU;Clang")
set(gcc_minimal_version "11")
set(clang_minimal_version "13")

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

function(AddSourceFiles TARGET_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "EXTENSIONS;DIRECTORIES")
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

        target_compile_options(${PROJECT_NAME} PRIVATE -march=native)
        if(DEBUG_BUILD AND RELEASE_BUILD)
                message(FATAL_ERROR "DEBUG_BUILD and RELEASE_BUILD")
        elseif(DEBUG_BUILD)
                target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_BUILD)
                target_compile_options(${PROJECT_NAME} PRIVATE -Og -ggdb)
        elseif(RELEASE_BUILD)
                target_compile_definitions(${PROJECT_NAME} PRIVATE RELEASE_BUILD)
                target_compile_options(${PROJECT_NAME} PRIVATE -O3)
        else()
                target_compile_options(${PROJECT_NAME} PRIVATE -O3)
        endif()

        target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -fno-rtti -fstrict-enums >)
        # target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>: -mllvm -inline-threshold=1000 >)

        SetCompilerWarnings("${all_files}")

endfunction()

function(AddBinaryFile)

        cmake_parse_arguments(PARSE_ARGV 0 ARG
                ""
                "INPUT_FILE;OUTPUT_FILE"
                "")

        add_custom_command(
                OUTPUT
                        "${ARG_OUTPUT_FILE}"
                COMMAND
                        "${PROJECT_BINARY_DIR}/create_str"
                        "bin" "${ARG_INPUT_FILE}" "${ARG_OUTPUT_FILE}"
                DEPENDS
                        "${ARG_INPUT_FILE}"
                        "${PROJECT_BINARY_DIR}/create_str"
                WORKING_DIRECTORY
                        "${PROJECT_BINARY_DIR}"
                VERBATIM
        )

endfunction()

function(AddShaderFile)

        cmake_parse_arguments(PARSE_ARGV 0 ARG
                ""
                "INPUT_FILE;OUTPUT_FILE"
                "GLOBAL_INCLUDE_DIRECTORIES;GLOBAL_INCLUDE_FILES;MACROS")

        unset(include_directory)
        unset(include_files)
        get_filename_component(include_directory "${ARG_INPUT_FILE}" DIRECTORY)
        unset(globbing_expressions_include)
        foreach(ext ${extensions_shaders_include})
                list(APPEND globbing_expressions_include "${include_directory}/*.${ext}")
        endforeach()
        file(GLOB_RECURSE include_files LIST_DIRECTORIES false ${globbing_expressions_include})

        unset(macro_list)
        foreach(macro ${ARG_MACROS})
                list(APPEND macro_list "-D${macro}")
        endforeach()

        unset(include_directories_list)
        list(APPEND include_directories_list "-I${include_directory}")
        foreach(dir ${ARG_GLOBAL_INCLUDE_DIRECTORIES})
                list(APPEND include_directories_list "-I${dir}")
        endforeach()

        get_filename_component(file_ext "${ARG_INPUT_FILE}" EXT)
        string(RANDOM LENGTH 10 random_string_glsl)
        set(output_glsl "${ARG_OUTPUT_FILE}_${random_string_glsl}${file_ext}")

        string(RANDOM LENGTH 10 random_string_sprv)
        set(output_sprv "${ARG_OUTPUT_FILE}_${random_string_sprv}.sprv")

        add_custom_command(
                OUTPUT
                        "${ARG_OUTPUT_FILE}"
                COMMAND
                        "${PROJECT_BINARY_DIR}/create_str"
                        "cat" "${ARG_INPUT_FILE}" "${output_glsl}"
                COMMAND
                        Vulkan::glslangValidator
                        ${include_directories_list}
                        "--target-env" "${shader_environment}"
                        ${macro_list}
                        "${output_glsl}"
                        "-o" "${output_sprv}"
                COMMAND
                        "${PROJECT_BINARY_DIR}/create_str"
                        "spr" "${output_sprv}" "${ARG_OUTPUT_FILE}"
                DEPENDS
                        "${ARG_INPUT_FILE}"
                        "${PROJECT_BINARY_DIR}/create_str"
                        ${include_files}
                        ${ARG_GLOBAL_INCLUDE_FILES}
                WORKING_DIRECTORY
                        "${PROJECT_BINARY_DIR}"
                VERBATIM
        )

endfunction()

function(AddIncludeFiles FILE_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG
                ""
                "OUTPUT_EXTENSION;OUTPUT_DIRECTORY"
                "EXTENSIONS;SOURCE;INCLUDE_DIRECTORIES;MACROS")
        if(NOT ARG_OUTPUT_EXTENSION)
                message(FATAL_ERROR "OUTPUT_EXTENSION is not specified")
        endif()
        if(NOT ARG_SOURCE)
                message(FATAL_ERROR "SOURCE is not specified")
        endif()
        if(NOT ARG_OUTPUT_DIRECTORY)
                message(FATAL_ERROR "OUTPUT_DIRECTORY is not specified")
        endif()
        if(ARG_UNPARSED_ARGUMENTS)
                message(FATAL_ERROR "Unparsed arguments ${ARG_UNPARSED_ARGUMENTS}")
        endif()

        #

        set(output_directory "${ARG_OUTPUT_DIRECTORY}")

        unset(binary_files)
        unset(vulkan_shaders)

        if("${FILE_TYPE}" STREQUAL "BINARY_FILES")
                if(ARG_MACROS)
                        message(FATAL_ERROR "Binary file macros are not supported")
                endif()
                if(ARG_INCLUDE_DIRECTORIES)
                        message(FATAL_ERROR "Binary file include are not supported")
                endif()
                set(binary_files TRUE)
                set(custom_target_prefix "custom_target_binary_files")
        elseif("${FILE_TYPE}" STREQUAL "VULKAN_SHADERS")
                set(vulkan_shaders TRUE)
                unset(macros)
                foreach(macro ${ARG_MACROS})
                        string(TOLOWER "${macro}" macro_lower)
                        if(macros)
                                string(APPEND macros "_")
                        endif()
                        string(APPEND macros "${macro_lower}")
                endforeach()
                set(custom_target_prefix "custom_target_vulkan_shaders")
                if(macros)
                        string(APPEND custom_target_prefix "_${macros}")
                        string(APPEND output_directory "/${macros}")
                endif()
        else()
                message(FATAL_ERROR "Unknown file type ${FILE_TYPE}")
        endif()

        #

        file(MAKE_DIRECTORY "${output_directory}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${ARG_OUTPUT_DIRECTORY}")

        #

        unset(global_include_directories)
        unset(global_include_files)
        if(ARG_INCLUDE_DIRECTORIES)
                unset(globbing_expressions_include)
                foreach(dir ${ARG_INCLUDE_DIRECTORIES})
                        set(dir "${PROJECT_SOURCE_DIR}/${dir}")
                        if(NOT IS_DIRECTORY "${dir}")
                                message(FATAL_ERROR "Include directory ${dir} is not a directory")
                        endif()
                        foreach(ext ${extensions_shaders_include})
                                list(APPEND globbing_expressions_include "${dir}/*.${ext}")
                        endforeach()
                endforeach()
                file(GLOB_RECURSE global_include_files LIST_DIRECTORIES false ${globbing_expressions_include})
                foreach(include_file ${global_include_files})
                        get_filename_component(dir "${include_file}" DIRECTORY)
                        list(APPEND global_include_directories "${dir}")
                endforeach()
        endif()

        #

        foreach(source ${ARG_SOURCE})

                set(source "${PROJECT_SOURCE_DIR}/${source}")
                if(IS_DIRECTORY "${source}")
                        if(NOT ARG_EXTENSIONS)
                                message(FATAL_ERROR "No extensions")
                        endif()
                        unset(globbing_expressions)
                        foreach(ext ${ARG_EXTENSIONS})
                                list(APPEND globbing_expressions "${source}/*.${ext}")
                        endforeach()
                        file(GLOB_RECURSE files LIST_DIRECTORIES false ${globbing_expressions})
                else()
                        set(files "${source}")
                endif()

                foreach(input_file ${files})

                        get_filename_component(file_name "${input_file}" NAME)

                        set(target_name "${custom_target_prefix}_${file_name}")
                        set(output_file "${output_directory}/${file_name}.${ARG_OUTPUT_EXTENSION}")

                        add_custom_target(${target_name} DEPENDS "${output_file}")
                        add_dependencies(${target_name} create_str)
                        add_dependencies(${PROJECT_NAME} ${target_name})

                        if(binary_files)

                                AddBinaryFile(
                                        INPUT_FILE ${input_file}
                                        OUTPUT_FILE ${output_file})

                        elseif(vulkan_shaders)

                                AddShaderFile(
                                        INPUT_FILE ${input_file}
                                        OUTPUT_FILE ${output_file}
                                        GLOBAL_INCLUDE_DIRECTORIES ${global_include_directories}
                                        GLOBAL_INCLUDE_FILES ${global_include_files}
                                        MACROS ${ARG_MACROS})

                        endif()

                endforeach()

        endforeach()

endfunction()

function(Check_C_Has_Int128)
        check_c_source_compiles("int main(void){__int128 v;}" C_HAS_INT128)
        if(NOT C_HAS_INT128)
                message(FATAL_ERROR "C doesn't have __int128")
        endif()
endfunction()

function(Check_CXX_Has_Int128)
        check_cxx_source_compiles("int main(){__int128 v;}" CXX_HAS_INT128)
        if(NOT CXX_HAS_INT128)
                message(FATAL_ERROR "C++ doesn't have __int128")
        endif()
endfunction()

##################################################

project(create_str C CXX)

AddSourceFiles(EXECUTABLE DIRECTORIES "src_str" EXTENSIONS ${extensions_all})

##################################################

project(math C CXX)

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
                        EXTENSIONS "ttf")

AddIncludeFiles(VULKAN_SHADERS
                        OUTPUT_DIRECTORY "${output_headers_directory}/shaders"
                        OUTPUT_EXTENSION "spr"
                        SOURCE "src/gpu"
                        INCLUDE_DIRECTORIES "src/shading"
                        EXTENSIONS ${extensions_shaders})

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
