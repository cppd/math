target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:
                -Wall
                -Wextra
                $<$<COMPILE_LANGUAGE:CXX>:
                        -Wabi
                        -Wcast-align
                        -Wcast-qual
                        -Wconditionally-supported
                        -Wctor-dtor-privacy
                        -Wdate-time
                        -Wdisabled-optimization
                        -Wduplicated-cond
                        -Wenum-compare
                        -Wformat=2
                        -Wformat-signedness
                        -Wframe-larger-than=10000
                        -Winit-self
                        -Wlarger-than=1000000
                        -Wlogical-op
                        -Wmissing-declarations
                        -Wmissing-format-attribute
                        -Wmissing-include-dirs
                        -Wnoexcept
                        -Wold-style-cast
                        -Woverloaded-virtual
                        -Wpacked
                        -Wredundant-decls
                        -Wshadow
                        -Wsign-promo
                        -Wstack-usage=10000
                        -Wstrict-aliasing=3
                        -Wstrict-overflow=1
                        -Wsuggest-attribute=format
                        -Wsuggest-attribute=noreturn
                        -Wsuggest-final-methods
                        -Wsuggest-final-types
                        -Wsuggest-override
                        -Wswitch-enum
                        -Wtrampolines
                        -Wundef
                        -Wunreachable-code
                        -Wunused
                        -Wunused-parameter
                        -Wvirtual-inheritance
                        -Wvla
                        -Wwrite-strings
                        -Wzero-as-null-pointer-constant
                >
                $<$<COMPILE_LANGUAGE:C>:
                        -Wabi
                        -Wbad-function-cast
                        -Wcast-align
                        -Wcast-qual
                        -Wc++-compat
                        -Wdate-time
                        -Wdisabled-optimization
                        -Wduplicated-cond
                        -Wenum-compare
                        -Wformat=2
                        -Wformat-signedness
                        -Wframe-larger-than=10000
                        -Winit-self
                        -Wjump-misses-init
                        -Wlarger-than=10000
                        -Wlogical-op
                        -Wmissing-declarations
                        -Wmissing-include-dirs
                        -Wmissing-prototypes
                        -Wnested-externs
                        -Wold-style-definition
                        -Wpacked
                        -Wredundant-decls
                        -Wshadow
                        -Wstack-usage=10000
                        -Wstrict-aliasing=3
                        -Wstrict-overflow=1
                        -Wstrict-prototypes
                        -Wswitch-enum
                        -Wtrampolines
                        -Wundef
                        -Wunreachable-code
                        -Wunsuffixed-float-constants
                        -Wunused
                        -Wunused-parameter
                        -Wvla
                        -Wwrite-strings
                >
        >
        $<$<CXX_COMPILER_ID:Clang>:
                -Weverything
                $<$<COMPILE_LANGUAGE:CXX>:
                        -Wno-c++98-compat
                        -Wno-c++98-compat-pedantic
                        -Wno-conversion
                        -Wno-double-promotion
                        -Wno-exit-time-destructors
                        -Wno-float-equal
                        -Wno-padded
                        -Wno-weak-vtables
                >
        >
)

#-Waggregate-return
#-Wconversion
#-Weffc++
#-Werror
#-Wfatal-errors
#-Wfloat-conversion
#-Wfloat-equal
#-Winline
#-Wmultiple-inheritance
#-Wnon-virtual-dtor
#-Wpadded
#-Wsign-conversion
#-Wsuggest-attribute=const
#-Wsuggest-attribute=pure
#-Wswitch-default
#-Wuseless-cast

#C:
#-Wtraditional
#-Wtraditional-conversion
#-Wdouble-promotion
