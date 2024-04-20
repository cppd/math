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

function(set_compiler_warnings source_files)

        target_compile_options(${PROJECT_NAME} PRIVATE
                $<$<CXX_COMPILER_ID:GNU>:
                        -Werror
                        -Wall
                        -Wextra
                        -Wcast-align
                        -Wcast-qual
                        -Wdate-time
                        -Wdisabled-optimization
                        -Wduplicated-branches
                        -Wduplicated-cond
                        -Wenum-compare
                        -Wformat-signedness
                        -Wformat=2
                        -Wframe-larger-than=20000
                        -Wimplicit-fallthrough=5
                        -Winit-self
                        -Wlarger-than=1000000
                        -Wlogical-op
                        -Wmissing-declarations
                        -Wmissing-format-attribute
                        -Wmissing-include-dirs
                        -Wpacked
                        -Wredundant-decls
                        -Wshadow=local
                        -Wstack-usage=20000
                        -Wstrict-aliasing=3
                        -Wstrict-overflow=1
                        -Wsuggest-attribute=format
                        -Wsuggest-attribute=noreturn
                        -Wswitch-enum
                        -Wtrampolines
                        -Wundef
                        -Wunreachable-code
                        -Wunused
                        -Wunused-parameter
                        -Wvla
                        -Wwrite-strings
                        $<$<COMPILE_LANGUAGE:CXX>:
                        -Wconditionally-supported
                        -Wctor-dtor-privacy
                        -Wextra-semi
                        -Wnon-virtual-dtor
                        -Wold-style-cast
                        -Woverloaded-virtual
                        -Wplacement-new=2
                        -Wsign-promo
                        -Wsuggest-final-methods
                        -Wsuggest-final-types
                        -Wsuggest-override
                        -Wvirtual-inheritance
                        -Wzero-as-null-pointer-constant
                        >
                        $<$<COMPILE_LANGUAGE:C>:
                        -Waggregate-return
                        -Wbad-function-cast
                        -Wc++-compat
                        -Wjump-misses-init
                        -Wmissing-prototypes
                        -Wnested-externs
                        -Wold-style-definition
                        -Wstrict-prototypes
                        -Wunsuffixed-float-constants
                        >
                >
                $<$<CXX_COMPILER_ID:Clang>:
                        -Werror
                        -Weverything
                        $<$<COMPILE_LANGUAGE:CXX>:
                        -Wno-c++20-compat-pedantic
                        -Wno-c++98-compat-pedantic
                        -Wno-cast-function-type-strict
                        -Wno-conversion
                        -Wno-double-promotion
                        -Wno-exit-time-destructors
                        -Wno-float-equal
                        -Wno-padded
                        -Wno-shadow-field-in-constructor
                        -Wno-shadow-uncaptured-local
                        -Wno-switch-default
                        -Wno-unsafe-buffer-usage
                        -Wno-weak-template-vtables
                        -Wno-weak-vtables

                        -Wno-ctad-maybe-unsupported
                        -Wno-undefined-func-template

                        # Qt
                        -Wno-reserved-identifier
                        >
                >
        )

        # foreach(f ${source_files})
        #         if (${f} MATCHES "^.+\.cpp$")
        #                 if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        #                         string(CONCAT flags)
        #                         set_source_files_properties(${f} PROPERTIES COMPILE_FLAGS ${flags})
        #                 endif()
        #         endif()
        # endforeach()

endfunction()
