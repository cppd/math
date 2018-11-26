#
# Copyright (C) 2017, 2018 Topological Manifold
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

cmake_minimum_required(VERSION 3.12)

##################################################

include("${CMAKE_SOURCE_DIR}/cmake/cmake_compiler_warnings.cmake")

##################################################

set(compilers "GNU;Clang")
set(gcc_minimal_version "8")
set(clang_minimal_version "7")

set(extensions_shaders "comp;frag;geom;vert;tesc;tese;glsl")
set(extensions_all "h;c;cpp;ui" ${extensions_shaders})
set(extensions_binary "ttf")

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

# При наличии enable_language(C CXX) можно указать стандарт для всех проектов
# -std=c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# -std=c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Чтобы было как "make VERBOSE=1"
# Добавлять после команды project или enable_language
set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

set(CMAKE_C_FLAGS)
set(CMAKE_CXX_FLAGS)
#if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
#        set(CMAKE_CXX_FLAGS "-stdlib=libc++")
#endif()

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

        if ("${TARGET_TYPE}" STREQUAL "EXECUTABLE")
                add_executable(${PROJECT_NAME} ${all_files})
        elseif ("${TARGET_TYPE}" STREQUAL "LIBRARY")
                add_library(${PROJECT_NAME} SHARED ${all_files})
        else()
                message(FATAL_ERROR "Unknown target type ${TARGET_TYPE}")
        endif()

        #   CMake при использовании target_include_directories вставляет -I,
        # а значит оба #include<> и #include"" будут тут искать, хотя надо -iquote,
        # чтобы поиск был только для #include"", не задевая поиск для #include<>.
        #   Если добавить -iquote к флагам компилятора, то перестают работать
        # зависимости и изменения в заголовочных файлах не замечаются.
        # string(CONCAT CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" " -iquote " "${PROJECT_SOURCE_DIR}/${dir}")
        # string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" " -iquote " "${PROJECT_SOURCE_DIR}/${dir}")
        #   Добавление самого верхнего уровня может привести к совпадениям с системными
        # файлами из-за использования -I в target_include_directories
        foreach(dir ${ARG_DIRECTORIES})
                target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/${dir}")
        endforeach()

        #target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
        #target_link_libraries(${PROJECT_NAME} -s)

        if(WIN32)
                target_compile_definitions(${PROJECT_NAME} PRIVATE -D__USE_MINGW_ANSI_STDIO=1)
        endif()

        target_compile_definitions(${PROJECT_NAME} PRIVATE -DNDEBUG)

        #-pedantic -pedantic-errors
        #-fopt-info-missed -fopt-info-vec-missed -O3 -ffast-math -march=native -pg

        target_link_libraries(${PROJECT_NAME} -pthread)
        target_compile_options(${PROJECT_NAME} PRIVATE -pthread)

        target_compile_options(${PROJECT_NAME} PRIVATE -Ofast -mavx -mfma)
        target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -fno-rtti -fstrict-enums >)

        target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:Clang>: -mllvm -inline-threshold=10000 >)

        SetCompilerWarnings("${all_files}")

endfunction()

function(AddIncludeFiles FILE_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG "" "PATH_CONTAINS_DIRECTORY;OUTPUT_EXTENSION" "EXTENSIONS;DIRECTORIES")
        if(NOT ARG_OUTPUT_EXTENSION)
                message(FATAL_ERROR "No output extension")
        endif()
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

        unset(BINARY_FILES)
        unset(OPENGL_SHADERS)
        unset(VULKAN_SHADERS)

        if ("${FILE_TYPE}" STREQUAL "BINARY_FILES")
                set(BINARY_FILES TRUE)
                set(custom_target_prefix "custom_target_binary_files")
                set(output_directory "${PROJECT_BINARY_DIR}/include_binary_files")
        elseif("${FILE_TYPE}" STREQUAL "OPENGL_SHADERS")
                set(OPENGL_SHADERS TRUE)
                set(custom_target_prefix "custom_target_opengl_shaders")
                set(output_directory "${PROJECT_BINARY_DIR}/include_opengl_shaders")
        elseif("${FILE_TYPE}" STREQUAL "VULKAN_SHADERS")
                set(VULKAN_SHADERS TRUE)
                set(custom_target_prefix "custom_target_vulkan_shaders")
                set(output_directory "${PROJECT_BINARY_DIR}/include_vulkan_shaders")
        else()
                message(FATAL_ERROR "Unknown file type ${FILE_TYPE}")
        endif()

        #

        file(GLOB files_to_delete LIST_DIRECTORIES false "${output_directory}/*")
        foreach(file ${files_to_delete})
                string(FIND "${file}" "${PROJECT_BINARY_DIR}/include_" position)
                if (${position} EQUAL 0)
                        file(REMOVE "${file}")
                else()
                        message(FATAL_ERROR "file name error ${file}")
                endif()
        endforeach()
        file(MAKE_DIRECTORY "${output_directory}")
        target_include_directories(${PROJECT_NAME} PRIVATE "${output_directory}")

        #

        foreach(dir ${ARG_DIRECTORIES})

                set(search_directory "${PROJECT_SOURCE_DIR}/${dir}")
                string(LENGTH "${search_directory}" search_directory_length)

                unset(globbing_expressions)
                foreach(ext ${ARG_EXTENSIONS})
                        list(APPEND globbing_expressions "${search_directory}/*.${ext}")
                endforeach()

                file(GLOB_RECURSE files LIST_DIRECTORIES false ${globbing_expressions})
                foreach(long_file_name ${files})

                        if (ARG_PATH_CONTAINS_DIRECTORY)
                                get_filename_component(file_dir "${long_file_name}" DIRECTORY)
                                string(SUBSTRING ${file_dir} ${search_directory_length} -1 substring_dir)
                                file(TO_CMAKE_PATH "${substring_dir}" cmake_style_dir)
                                string(FIND "${cmake_style_dir}/" "/${ARG_PATH_CONTAINS_DIRECTORY}/" position)
                                if (${position} LESS 0)
                                        continue()
                                endif()
                        endif()

                        get_filename_component(file_name "${long_file_name}" NAME)
                        set(target_name "${custom_target_prefix}_${file_name}")
                        set(output_file "${output_directory}/${file_name}.${ARG_OUTPUT_EXTENSION}")

                        add_custom_target(${target_name} DEPENDS "${output_file}")
                        add_dependencies(${target_name} create_str)
                        add_dependencies(${PROJECT_NAME} ${target_name})

                        if (BINARY_FILES)
                                add_custom_command(
                                        OUTPUT  "${output_file}"
                                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "bin" "${long_file_name}" "${output_file}"
                                        DEPENDS "${long_file_name}" "${PROJECT_BINARY_DIR}/create_str"
                                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                                        VERBATIM
                                )
                        endif()

                        if (OPENGL_SHADERS)
                                add_custom_command(
                                        OUTPUT  "${output_file}"
                                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "str" "${long_file_name}" "${output_file}"
                                        DEPENDS "${long_file_name}" "${PROJECT_BINARY_DIR}/create_str"
                                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                                        VERBATIM
                                )
                        endif()

                        if(VULKAN_SHADERS)
                                string(RANDOM LENGTH 10 random_string)
                                set(output_sprv "${output_file}.${random_string}.${ARG_OUTPUT_EXTENSION}")
                                add_custom_command(
                                        OUTPUT  "${output_sprv}"
                                        COMMAND "glslangValidator" "-V" "${long_file_name}" "-o" "${output_sprv}"
                                        DEPENDS "${long_file_name}"
                                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                                        VERBATIM
                                )
                                add_custom_command(
                                        OUTPUT  "${output_file}"
                                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "spr" "${output_sprv}" "${output_file}"
                                        DEPENDS "${output_sprv}" "${PROJECT_BINARY_DIR}/create_str"
                                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                                        VERBATIM
                                )
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

# Clang 5 не работает с std::variant из libstdc++
function(Check_CXX_Variant_Works)
        unset(CXX_VARIANT_WORKS CACHE)
        check_cxx_source_compiles(
                "
                #include <utility>
                #include <variant>
                int main()
                {
                        std::variant<int, double> a(1.0);
                        std::get<double>(a);
                }
                "
                CXX_VARIANT_WORKS)
endfunction()

# libc++ 7 не имеет std::beta
function(Check_CXX_Beta_Function_Works)
        unset(CXX_BETA_FUNCTION_WORKS CACHE)
        check_cxx_source_compiles(
                "
                #include <cmath>
                int main()
                {
                        std::beta(1, 1);
                }
                "
                CXX_BETA_FUNCTION_WORKS)
endfunction()

##################################################

project(create_str C CXX)

AddSourceFiles(EXECUTABLE DIRECTORIES src_str EXTENSIONS ${extensions_all})

##################################################

project(create_gl C CXX)

AddSourceFiles(EXECUTABLE DIRECTORIES src_gl EXTENSIONS ${extensions_all})

##################################################

project(math C CXX)

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

AddIncludeFiles(VULKAN_SHADERS
                        OUTPUT_EXTENSION "spr"
                        DIRECTORIES "src"
                        PATH_CONTAINS_DIRECTORY "vulkan"
                        EXTENSIONS ${extensions_shaders})

AddIncludeFiles(OPENGL_SHADERS
                        OUTPUT_EXTENSION "str"
                        DIRECTORIES "src"
                        PATH_CONTAINS_DIRECTORY "opengl"
                        EXTENSIONS ${extensions_shaders})
AddIncludeFiles(OPENGL_SHADERS
                        OUTPUT_EXTENSION "str"
                        DIRECTORIES "src/gpgpu"
                        EXTENSIONS ${extensions_shaders})

AddIncludeFiles(BINARY_FILES
                        OUTPUT_EXTENSION "bin"
                        DIRECTORIES "extern/resources"
                        EXTENSIONS ${extensions_binary})

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/extern/include")

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

##################################################

Check_CXX_Variant_Works()
if(NOT CXX_VARIANT_WORKS)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DSTD_VARIANT_NOT_FOUND)
endif()

Check_CXX_Beta_Function_Works()
if(NOT CXX_BETA_FUNCTION_WORKS)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DSTD_BETA_FUNCTION_NOT_FOUND)
endif()

##################################################

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/SFML")
find_package(SFML REQUIRED COMPONENTS system window graphics)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${SFML_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${SFML_LIBRARIES})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GMP")
find_package(GMP REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GMP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GMP_C_LIBRARIES} ${GMP_CXX_LIBRARIES})

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

find_package(Qt5Widgets REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Qt5Widgets_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Qt5Widgets_LIBRARIES})

find_package(Freetype REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARIES})

find_package(X11 REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${X11_X11_INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} ${X11_X11_LIB})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GLFW")
find_package(GLFW REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GLFW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/FFTW")
find_package(FFTW)
if(FFTW_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DFFTW_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${FFTW_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${FFTW_LIBRARIES} ${FFTW_THREAD_LIBRARIES})
endif()

find_package(CUDA)
if(CUDA_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DCUDA_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CUDA_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${CUDA_LIBRARIES} ${CUDA_CUFFT_LIBRARIES})
endif()

#if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
#target_link_libraries(${PROJECT_NAME} quadmath)
#endif()

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Eigen3")
##set(EIGEN3_ROOT "")
##set(EIGEN3_ROOT_DIR "")
#find_package(Eigen3 NO_MODULE)
#if(EIGEN3_FOUND)
#        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${EIGEN3_INCLUDE_DIR})
#        target_compile_definitions(${PROJECT_NAME} PRIVATE -DEIGEN_MPL2_ONLY)
#endif()
