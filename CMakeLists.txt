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

#make VERBOSE=1 -j 8

#set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS) # чтобы убрать -rdynamic
#set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS) # чтобы убрать -rdynamic

###################################################

if(true)
        set(CMAKE_C_COMPILER   "gcc")
        set(CMAKE_CXX_COMPILER "g++")
else()
        set(CMAKE_C_COMPILER   "clang-3.9")
        set(CMAKE_CXX_COMPILER "clang++-3.9")
endif()

###################################################

macro (IncludeSourceAndSetCompilerFlags dir_name executable)

        set(CMAKE_C_FLAGS)
        set(CMAKE_CXX_FLAGS)

        # Чтобы было как "make VERBOSE=1"
        # Добавлять после команды project
        set(CMAKE_VERBOSE_MAKEFILE TRUE)

        # -std=c11
        set(CMAKE_C_STANDARD 11)
        set(CMAKE_C_STANDARD_REQUIRED ON)
        set(CMAKE_C_EXTENSIONS OFF)

        # Указать стандарт C++14 для cmake с помощью CMAKE_CXX_STANDARD вместо явного -std=c++14
        set(CMAKE_CXX_STANDARD 14)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        set(CMAKE_CXX_EXTENSIONS OFF)
        # Перевести с++14 на с++17, так как cmake версии 3.7.2 не поддерживает set(CMAKE_CXX_STANDARD 17)
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
                set(CMAKE_CXX14_STANDARD_COMPILE_OPTION "-std=c++17")
                set(CMAKE_CXX14_EXTENSION_COMPILE_OPTION "-std=gnu++17")
        elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
                set(CMAKE_CXX14_STANDARD_COMPILE_OPTION "-std=c++1z")
                set(CMAKE_CXX14_EXTENSION_COMPILE_OPTION "-std=gnu++1z")
        else()
                message(FATAL_ERROR "Неизвестный компилятор")
        endif()

        if (true)
                set(extensions h;c;cpp;ui;comp;frag;geom;vert;tesc;tese)
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

        #C: -Wtraditional -Wtraditional-conversion -Wdouble-promotion
        #-pedantic -pedantic-errors
        #-Winline -Waggregate-return -Weffc++ -Wfloat-equal -Wnon-virtual-dtor
        #-Wpadded -Wconversion -Wsign-conversion -Wfloat-conversion
        #-fopt-info-missed -fopt-info-vec-missed -S -masm=intel
        #-Wuseless-cast -Wswitch-default -Werror -Wfatal-errors
        #-pthread -march=native -pg -mavx
        #-funsafe-loop-optimizations -Wunsafe-loop-optimizations
        #-Wmultiple-inheritance -Wsuggest-attribute=pure -Wsuggest-attribute=const
        #-O3 -ffast-math

        target_link_libraries(${PROJECT_NAME} -pthread)
        target_compile_options(${PROJECT_NAME} PRIVATE -pthread)

        target_compile_options(${PROJECT_NAME} PRIVATE
                -Ofast -mavx -mfma #-march=native
                $<$<COMPILE_LANGUAGE:CXX>:
                        -fno-rtti -fstrict-enums
                >
                $<$<CXX_COMPILER_ID:GNU>:
                        -funsafe-loop-optimizations
                        -Wall -Wextra
                        $<$<COMPILE_LANGUAGE:CXX>:
                                -Wabi -Wcast-align -Wcast-qual -Wdisabled-optimization -Wswitch-enum
                                -Wformat=2 -Wformat-signedness -Wunused -Wunused-parameter -Wwrite-strings
                                -Wframe-larger-than=10000 -Winit-self -Wlarger-than=1000000 -Wlogical-op
                                -Wmissing-include-dirs -Wmissing-noreturn -Wredundant-decls -Wshadow -Wnoexcept
                                -Wstack-usage=10000 -Wstrict-aliasing=3 -Wstrict-overflow=1 -Wtrampolines
                                -Wundef -Wunreachable-code -Wenum-compare -Wvla -Wold-style-cast -Wsign-promo
                                -Woverloaded-virtual -Wzero-as-null-pointer-constant -Wconditionally-supported
                                -Wduplicated-cond -Wdate-time -Wsuggest-final-types -Wsuggest-final-methods
                                -Wsuggest-override -Wsuggest-attribute=noreturn -Wsuggest-attribute=format
                                -Wmissing-format-attribute -Wmissing-declarations -Wpacked -Wdisabled-optimization
                                -Wvirtual-inheritance -Wctor-dtor-privacy
                        >
                        $<$<COMPILE_LANGUAGE:C>:
                                -Wabi -Wcast-align -Wcast-qual -Wdisabled-optimization -Wswitch-enum
                                -Wformat=2 -Wformat-signedness -Wunused -Wunused-parameter -Wwrite-strings
                                -Wframe-larger-than=10000 -Winit-self -Wlarger-than=10000 -Wlogical-op
                                -Wmissing-include-dirs -Wmissing-noreturn -Wredundant-decls -Wshadow
                                -Wstack-usage=10000 -Wstrict-aliasing=3 -Wstrict-overflow=1 -Wtrampolines
                                -Wundef -Wunreachable-code -Wenum-compare -Wvla -Wbad-function-cast
                                -Wjump-misses-init -Wstrict-prototypes -Wold-style-definition -Wc++-compat
                                -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wdate-time
                                -Wunsuffixed-float-constants -Wduplicated-cond -Wpacked -Wdisabled-optimization
                        >
                >
                $<$<CXX_COMPILER_ID:Clang>:
                        -Weverything
                        $<$<COMPILE_LANGUAGE:CXX>:
                                -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-conversion
                                -Wno-weak-vtables -Wno-float-equal -Wno-double-promotion
                                -Wno-exit-time-destructors
                        >
                >
        )

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

###########################################################################

project(create_str C CXX)

IncludeSourceAndSetCompilerFlags(src_str true)

###########################################################################

project(create_gl C CXX)

IncludeSourceAndSetCompilerFlags(src_gl true)

###########################################################################

project(math C CXX)

# Qt
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOMOC_MOC_OPTIONS some_options)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

IncludeSourceAndSetCompilerFlags(src true)

#target_compile_options(${PROJECT_NAME} PRIVATE -S -masm=intel)

AddShaders(src str "comp;frag;geom;vert;tesc;tese")
AddShaders(extern/resources bin "ttf")

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/extern/include")

target_compile_definitions(${PROJECT_NAME} PRIVATE -DGLM_FORCE_NO_CTOR_INIT -DGLM_FORCE_CXX14 -DGLM_FORCE_EXPLICIT_CTOR)

#####################

target_link_libraries(${PROJECT_NAME} stdc++fs)

#if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
#target_link_libraries(${PROJECT_NAME} quadmath)
#endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/SFML")
#set(SFML_ROOT "")
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

#set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Eigen3")
##set(EIGEN3_ROOT "")
##set(EIGEN3_ROOT_DIR "")
#find_package(Eigen3 3.3.2 NO_MODULE)
#if(EIGEN3_FOUND)
#        target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${EIGEN3_INCLUDE_DIR})
#        target_compile_definitions(${PROJECT_NAME} PRIVATE -DEIGEN_MPL2_ONLY)
#endif()

