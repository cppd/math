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

cmake_minimum_required(VERSION 3.11)

##################################################

include("${CMAKE_SOURCE_DIR}/cmake/cmake_compiler_warnings.cmake")

##################################################

set(compilers "GNU;Clang")
set(gcc_minimal_version "7.3")
set(clang_minimal_version "6")
set(sfml_minimal_version "2.4.2")

set(extensions_shaders "comp;frag;geom;vert;tesc;tese;glsl")
set(extensions_all "h;c;cpp;ui" ${extensions_shaders})

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

##################################################

function(IncludeSourceAndSetCompilerFlags dir_name executable extensions)

        if("${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
                message(FATAL_ERROR "In-source build")
        endif()

        set(CMAKE_C_FLAGS)
        set(CMAKE_CXX_FLAGS)

        foreach(ext ${extensions})
                list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir_name}/*.${ext}")
        endforeach()

        file(GLOB_RECURSE source_files LIST_DIRECTORIES false ${all_globbing_expressions})

        if(${executable})
                add_executable(${PROJECT_NAME} ${source_files})
        else()
                add_library(${PROJECT_NAME} SHARED ${source_files})
        endif()

        #   CMake при использовании target_include_directories вставляет -I,
        # а значит оба #include<> и #include"" будут тут искать, хотя надо -iquote,
        # чтобы поиск был только для #include"", не задевая поиск для #include<>.
        #   Если добавить -iquote к флагам компилятора, то перестают работать
        # зависимости и изменения в заголовочных файлах не замечаются.
        # string(CONCAT CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" " -iquote " "${PROJECT_SOURCE_DIR}/${dir_name}")
        # string(CONCAT CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" " -iquote " "${PROJECT_SOURCE_DIR}/${dir_name}")
        #   Добавление самого верхнего уровня может привести к совпадениям с системными
        # файлами из-за использования -I в target_include_directories
        target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/${dir_name}")

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

        SetCompilerWarnings("${source_files}")

endfunction()

function(AddBinaryFiles dir_name extensions)

        foreach(ext ${extensions})
                list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir_name}/*.${ext}")
        endforeach()

        file(GLOB_RECURSE binary_files LIST_DIRECTORIES false ${all_globbing_expressions})

        foreach(long_file_name ${binary_files})

                get_filename_component(file_name ${long_file_name} NAME)

                set(file_bin "${PROJECT_BINARY_DIR}/${file_name}.bin")

                add_custom_command(
                        OUTPUT  "${file_bin}"
                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "bin" "${long_file_name}" "${file_bin}"
                        DEPENDS "${long_file_name}" "${PROJECT_BINARY_DIR}/create_str"
                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                        VERBATIM
                        )

                set(target_name "custom_target_bin_${file_name}")
                add_custom_target(${target_name} DEPENDS "${file_bin}")
                add_dependencies(${target_name} create_str)
                add_dependencies(${PROJECT_NAME} ${target_name})

        endforeach()

        target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

endfunction()

function(AddOpenGLShaders dir_name extensions)

        foreach(ext ${extensions})
                list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir_name}/*.${ext}")
        endforeach()

        file(GLOB_RECURSE shader_files LIST_DIRECTORIES false ${all_globbing_expressions})

        foreach(long_shader_name ${shader_files})

                get_filename_component(shader_name ${long_shader_name} NAME)

                set(shader_str "${PROJECT_BINARY_DIR}/${shader_name}.str")

                add_custom_command(
                        OUTPUT  "${shader_str}"
                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "str" "${long_shader_name}" "${shader_str}"
                        DEPENDS "${long_shader_name}" "${PROJECT_BINARY_DIR}/create_str"
                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                        VERBATIM
                        )

                set(target_name "custom_target_str_${shader_name}")
                add_custom_target(${target_name} DEPENDS "${shader_str}")
                add_dependencies(${target_name} create_str)
                add_dependencies(${PROJECT_NAME} ${target_name})

        endforeach()

        target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

endfunction()

function(AddVulkanShaders dir_name extensions)

        foreach(ext ${extensions})
                list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir_name}/*.${ext}")
        endforeach()

        file(GLOB_RECURSE shader_files LIST_DIRECTORIES false ${all_globbing_expressions})

        foreach(long_shader_name ${shader_files})

                get_filename_component(shader_name ${long_shader_name} NAME)

                set(shader_spr "${PROJECT_BINARY_DIR}/${shader_name}.spr")
                set(shader_bin "${PROJECT_BINARY_DIR}/${shader_name}.bin")

                add_custom_command(
                        OUTPUT  "${shader_bin}"
                        COMMAND "glslangValidator" "-V" "${long_shader_name}" "-o" "${shader_bin}"
                        DEPENDS "${long_shader_name}"
                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                        VERBATIM
                        )

                add_custom_command(
                        OUTPUT  "${shader_spr}"
                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "spr" "${shader_bin}" "${shader_spr}"
                        DEPENDS "${shader_bin}" "${PROJECT_BINARY_DIR}/create_str"
                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                        VERBATIM
                        )

                set(target_name "custom_target_spr_${shader_name}")
                add_custom_target(${target_name} DEPENDS "${shader_spr}")
                add_dependencies(${target_name} create_str)
                add_dependencies(${PROJECT_NAME} ${target_name})

        endforeach()

        target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

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

##################################################

project(create_str C CXX)

IncludeSourceAndSetCompilerFlags(src_str true "${extensions_all}")

##################################################

project(create_gl C CXX)

IncludeSourceAndSetCompilerFlags(src_gl true "${extensions_all}")

##################################################

project(math C CXX)

# Qt
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOMOC_MOC_OPTIONS some_options)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

Check_C_Has_Int128()
Check_CXX_Has_Int128()

IncludeSourceAndSetCompilerFlags(src true "${extensions_all}")

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

AddOpenGLShaders(src "${extensions_shaders}")
AddBinaryFiles(extern/resources "ttf")

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/extern/include")

##################################################

Check_CXX_Variant_Works()
if(NOT CXX_VARIANT_WORKS)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DSTD_VARIANT_NOT_FOUND)
endif()

##################################################

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/SFML")
find_package(SFML ${sfml_minimal_version} REQUIRED COMPONENTS system window graphics)
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

find_package(Vulkan)
if(Vulkan_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DVULKAN_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})
        AddVulkanShaders(src/show/renderer_vulkan "${extensions_shaders}")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GLFW")
find_package(GLFW)
if(GLFW_FOUND)
        target_compile_definitions(${PROJECT_NAME} PRIVATE -DGLFW_FOUND)
        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GLFW_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
endif()

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

#target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/ library header dir")
#target_link_libraries(${PROJECT_NAME} -L"${CMAKE_BINARY_DIR}" -l library_project)
#add_dependencies(${PROJECT_NAME} library_project)

#if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
#target_link_libraries(${PROJECT_NAME} quadmath)
#endif()

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Eigen3")
##set(EIGEN3_ROOT "")
##set(EIGEN3_ROOT_DIR "")
#find_package(Eigen3 3.3.2 NO_MODULE)
#if(EIGEN3_FOUND)
#        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${EIGEN3_INCLUDE_DIR})
#        target_compile_definitions(${PROJECT_NAME} PRIVATE -DEIGEN_MPL2_ONLY)
#endif()

# чтобы убрать -rdynamic
#set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
#set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)
