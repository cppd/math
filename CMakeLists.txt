#
# Copyright (C) 2017 Topological Manifold
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

cmake_minimum_required(VERSION 3.7.2)

##################################################

enable_language(C CXX)

set(Compilers "GNU;Clang")

if(NOT CMAKE_C_COMPILER_ID)
         message(FATAL_ERROR "Empty C compiler id")
endif()
if(NOT CMAKE_CXX_COMPILER_ID)
         message(FATAL_ERROR "Empty C++ compiler id")
endif()
if(NOT (${CMAKE_C_COMPILER_ID} IN_LIST Compilers))
        message(WARNING "Not supported C compiler ${CMAKE_C_COMPILER_ID}")
endif()
if(NOT (${CMAKE_CXX_COMPILER_ID} IN_LIST Compilers))
        message(WARNING "Not supported C++ compiler ${CMAKE_CXX_COMPILER_ID}")
endif()
if((${CMAKE_C_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS 7.1))
        message(WARNING "Not supported GCC C compiler version ${CMAKE_C_COMPILER_VERSION}")
endif()
if((${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 7.1))
        message(WARNING "Not supported GCC C++ compiler version ${CMAKE_C_COMPILER_VERSION}")
endif()

# При наличии enable_language(C CXX) можно указать стандарт для всех проектов
# -std=c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
# Указать стандарт C++14 для cmake с помощью CMAKE_CXX_STANDARD вместо явного -std=c++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Перевести с++14 на с++17, так как cmake версии 3.7.2 не поддерживает set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX14_STANDARD_COMPILE_OPTION "-std=c++1z")
set(CMAKE_CXX14_EXTENSION_COMPILE_OPTION "-std=gnu++1z")

# Чтобы было как "make VERBOSE=1"
# Добавлять после команды project или enable_language
set(CMAKE_VERBOSE_MAKEFILE TRUE)

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

##################################################

macro (IncludeSourceAndSetCompilerFlags dir_name executable)

        if ("${PROJECT_BINARY_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
                message(FATAL_ERROR "In-source build")
        endif ()

        set(CMAKE_C_FLAGS)
        set(CMAKE_CXX_FLAGS)

        if (true)
                set(extensions "h;c;cpp;ui;comp;frag;geom;vert;tesc;tese")
                unset(all_globbing_expressions)
                foreach(ext ${extensions})
                        list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir_name}/*.${ext}")
                endforeach()
                file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false ${all_globbing_expressions})
        else()
                file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false ${PROJECT_SOURCE_DIR}/${dir_name}/*)
        endif()

        if (${executable})
                add_executable(${PROJECT_NAME} ${SRC_FILES})
        else()
                add_library(${PROJECT_NAME} SHARED ${SRC_FILES})
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

        include("${CMAKE_SOURCE_DIR}/cmake/cmake_compiler_warnings.cmake")

endmacro()

macro(AddShaders dir_name shader_type extensions)

        unset(all_globbing_expressions)
        foreach(ext ${extensions})
                list(APPEND all_globbing_expressions "${PROJECT_SOURCE_DIR}/${dir_name}/*.${ext}")
        endforeach()
        file(GLOB_RECURSE SHADER_FILES LIST_DIRECTORIES false ${all_globbing_expressions})

        foreach(long_shader_name ${SHADER_FILES})
                get_filename_component(shader_name ${long_shader_name} NAME)

                add_custom_command(
                        OUTPUT  "${PROJECT_BINARY_DIR}/${shader_name}.${shader_type}"
                        COMMAND "${PROJECT_BINARY_DIR}/create_str" "${shader_type}" "${long_shader_name}"
                                                        "${PROJECT_BINARY_DIR}/${shader_name}.${shader_type}"
                        DEPENDS "${long_shader_name}" "${PROJECT_BINARY_DIR}/create_str"
                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                        )

                string(CONCAT TARGET_NAME "custom_target_" "${shader_name}")
                add_custom_target(${TARGET_NAME} DEPENDS "${PROJECT_BINARY_DIR}/${shader_name}.${shader_type}")
                add_dependencies(${TARGET_NAME} create_str)
                add_dependencies(${PROJECT_NAME} ${TARGET_NAME})
        endforeach()

        target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

endmacro()

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

IncludeSourceAndSetCompilerFlags(src_str true)

##################################################

project(create_gl C CXX)

IncludeSourceAndSetCompilerFlags(src_gl true)

##################################################

project(math C CXX)

# Qt
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOMOC_MOC_OPTIONS some_options)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

Check_C_Has_Int128()
Check_CXX_Has_Int128()

IncludeSourceAndSetCompilerFlags(src true)

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

AddShaders(src str "comp;frag;geom;vert;tesc;tese")
AddShaders(extern/resources bin "ttf")

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/extern/include")

target_compile_definitions(${PROJECT_NAME} PRIVATE -DGLM_FORCE_NO_CTOR_INIT -DGLM_FORCE_CXX14 -DGLM_FORCE_EXPLICIT_CTOR)

##################################################

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/SFML")
find_package(SFML 2.4.1 COMPONENTS system window graphics REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${SFML_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${SFML_LIBRARIES})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GMP")
find_package(GMP REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GMP_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GMP_C_LIBRARIES} ${GMP_CXX_LIBRARIES})

find_package(OpenGL REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${OPENGL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})

find_package(Qt5Widgets REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Qt5Widgets_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Qt5Widgets_LIBRARIES})

find_package(Freetype REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARIES})

find_package(X11 REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${X11_X11_INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} ${X11_X11_LIB})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/FFTW")
find_package(FFTW)
if (FFTW_FOUND)
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

# В проект включены исходные тексты этой библиотеки, поэтому подключать не надо
#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/GLM")
#find_package(GLM REQUIRED)
#target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${GLM_INCLUDE_DIRS})

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
