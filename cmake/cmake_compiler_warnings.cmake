function(SetCompilerWarnings source_files)

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
                        -Wformat=2
                        -Wformat-signedness
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
                        # VK_NULL_HANDLE
                        # -Wzero-as-null-pointer-constant
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
                        -Wno-c++98-compat
                        -Wno-c++98-compat-pedantic
                        -Wno-conversion
                        -Wno-double-promotion
                        -Wno-exit-time-destructors
                        -Wno-float-equal
                        -Wno-padded
                        -Wno-shadow-field-in-constructor
                        -Wno-weak-vtables
                        -Wno-weak-template-vtables

                        -Wno-undefined-func-template
                        -Wno-ctad-maybe-unsupported

                        # Из-за файлов Qt отключить для всех файлов
                        # с последующим включением для файлов проекта
                        -Wno-redundant-parens
                        -Wno-extra-semi-stmt
                        >
                >
        )

        # Предупреждения для файлов проекта и не для файлов Qt
        foreach(f ${source_files})

                if (${f} MATCHES "^.+\.cpp$")

                        if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
                                string(CONCAT flags
                                        "-Wredundant-parens"
                                        " -Wextra-semi-stmt"
                                )
                                set_source_files_properties(${f} PROPERTIES COMPILE_FLAGS ${flags})
                        endif()

                endif()

        endforeach()

endfunction()
