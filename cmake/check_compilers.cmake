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

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)

function(check_compiler_versions)

        cmake_parse_arguments(PARSE_ARGV 0 ARG
                ""
                "GCC_MINIMAL_VERSION;CLANG_MINIMAL_VERSION"
                "")

        if(NOT ARG_GCC_MINIMAL_VERSION)
                message(FATAL_ERROR "GCC_MINIMAL_VERSION is not specified")
        endif()
        if(NOT ARG_CLANG_MINIMAL_VERSION)
                message(FATAL_ERROR "CLANG_MINIMAL_VERSION is not specified")
        endif()
        if(ARG_UNPARSED_ARGUMENTS)
                message(FATAL_ERROR "Unparsed arguments ${ARG_UNPARSED_ARGUMENTS}")
        endif()

        if(NOT CMAKE_C_COMPILER_ID)
                 message(FATAL_ERROR "Empty C compiler id")
        endif()
        if(NOT CMAKE_CXX_COMPILER_ID)
                 message(FATAL_ERROR "Empty C++ compiler id")
        endif()

        set(compilers "GNU;Clang")

        if(NOT (${CMAKE_C_COMPILER_ID} IN_LIST compilers))
                message(FATAL_ERROR "Not supported C compiler ${CMAKE_C_COMPILER_ID}")
        endif()
        if(NOT (${CMAKE_CXX_COMPILER_ID} IN_LIST compilers))
                message(FATAL_ERROR "Not supported C++ compiler ${CMAKE_CXX_COMPILER_ID}")
        endif()

        if((${CMAKE_C_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${ARG_GCC_MINIMAL_VERSION}))
                message(FATAL_ERROR "Not supported GCC C compiler version ${CMAKE_C_COMPILER_VERSION}")
        endif()
        if((${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${ARG_GCC_MINIMAL_VERSION}))
                message(FATAL_ERROR "Not supported GCC C++ compiler version ${CMAKE_CXX_COMPILER_VERSION}")
        endif()

        if((${CMAKE_C_COMPILER_ID} STREQUAL "Clang") AND (${CMAKE_C_COMPILER_VERSION} VERSION_LESS ${ARG_CLANG_MINIMAL_VERSION}))
                message(FATAL_ERROR "Not supported Clang C compiler version ${CMAKE_C_COMPILER_VERSION}")
        endif()
        if((${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang") AND (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${ARG_CLANG_MINIMAL_VERSION}))
                message(FATAL_ERROR "Not supported Clang C++ compiler version ${CMAKE_CXX_COMPILER_VERSION}")
        endif()

endfunction()

function(check_compiler_int128)
        check_c_source_compiles("int main(){__int128 v;}" C_HAS_INT128)
        if(NOT C_HAS_INT128)
                message(FATAL_ERROR "C doesn't have __int128")
        endif()
        check_cxx_source_compiles("int main(){__int128 v;}" CXX_HAS_INT128)
        if(NOT CXX_HAS_INT128)
                message(FATAL_ERROR "C++ doesn't have __int128")
        endif()
endfunction()


