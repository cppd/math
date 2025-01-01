#
# Copyright (C) 2017-2025 Topological Manifold
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

function(add_binary_file)

        cmake_parse_arguments(PARSE_ARGV 0 ARG
                ""
                "INPUT_FILE;OUTPUT_FILE;CREATE_STR"
                "")

        add_custom_command(
                OUTPUT
                        "${ARG_OUTPUT_FILE}"
                COMMAND
                        "${ARG_CREATE_STR}"
                        "bin" "${ARG_INPUT_FILE}" "${ARG_OUTPUT_FILE}"
                DEPENDS
                        "${ARG_INPUT_FILE}"
                        "${ARG_CREATE_STR}"
                WORKING_DIRECTORY
                        "${PROJECT_BINARY_DIR}"
                VERBATIM
        )

endfunction()

function(add_shader_file)

        cmake_parse_arguments(PARSE_ARGV 0 ARG
                ""
                "INPUT_FILE;OUTPUT_FILE;SHADER_ENVIRONMENT;CREATE_STR;GLSL_COMPILER"
                "INCLUDE_DIRECTORIES;INCLUDE_EXTENSIONS;MACROS")

        get_filename_component(include_directory "${ARG_INPUT_FILE}" DIRECTORY)
        unset(include_directories)
        list(APPEND include_directories "${include_directory}")
        if(ARG_INCLUDE_DIRECTORIES)
                foreach(dir ${ARG_INCLUDE_DIRECTORIES})
                        list(APPEND include_directories "${dir}")
                endforeach()
        endif()
        list(REMOVE_DUPLICATES include_directories)

        unset(globbing_expressions_include)
        foreach(dir ${include_directories})
                foreach(ext ${ARG_INCLUDE_EXTENSIONS})
                        list(APPEND globbing_expressions_include "${dir}/*.${ext}")
                endforeach()
        endforeach()
        unset(include_files)
        file(GLOB_RECURSE include_files LIST_DIRECTORIES false ${globbing_expressions_include})
        list(REMOVE_DUPLICATES include_files)
        list(REMOVE_ITEM include_files ${ARG_INPUT_FILE})

        unset(macro_list)
        foreach(macro ${ARG_MACROS})
                list(APPEND macro_list "-D${macro}")
        endforeach()

        unset(include_directories_list)
        foreach(dir ${include_directories})
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
                        "${ARG_CREATE_STR}"
                        "cat" "${ARG_INPUT_FILE}" "${output_glsl}"
                COMMAND
                        ${ARG_GLSL_COMPILER}
                        ${include_directories_list}
                        "--target-env" "${ARG_SHADER_ENVIRONMENT}"
                        ${macro_list}
                        "${output_glsl}"
                        "-o" "${output_sprv}"
                COMMAND
                        "${ARG_CREATE_STR}"
                        "spr" "${output_sprv}" "${ARG_OUTPUT_FILE}"
                DEPENDS
                        "${ARG_INPUT_FILE}"
                        "${ARG_CREATE_STR}"
                        ${include_files}
                WORKING_DIRECTORY
                        "${PROJECT_BINARY_DIR}"
                VERBATIM
        )

endfunction()

function(add_include_files FILE_TYPE)

        cmake_parse_arguments(PARSE_ARGV 1 ARG
                ""
                "OUTPUT_EXTENSION;OUTPUT_DIRECTORY;SHADER_ENVIRONMENT;CREATE_STR;GLSL_COMPILER"
                "EXTENSIONS;SOURCE;INCLUDE_DIRECTORIES;INCLUDE_EXTENSIONS;MACROS")

        if(NOT ARG_OUTPUT_EXTENSION)
                message(FATAL_ERROR "OUTPUT_EXTENSION is not specified")
        endif()
        if(NOT ARG_SOURCE)
                message(FATAL_ERROR "SOURCE is not specified")
        endif()
        if(NOT ARG_OUTPUT_DIRECTORY)
                message(FATAL_ERROR "OUTPUT_DIRECTORY is not specified")
        endif()
        if(NOT ARG_CREATE_STR)
                message(FATAL_ERROR "CREATE_STR is not specified")
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
                        message(FATAL_ERROR "Binary file MACROS is not supported")
                endif()
                if(ARG_INCLUDE_DIRECTORIES)
                        message(FATAL_ERROR "Binary file INCLUDE_DIRECTORIES is not supported")
                endif()
                if(ARG_INCLUDE_EXTENSIONS)
                        message(FATAL_ERROR "Binary file INCLUDE_EXTENSIONS is not supported")
                endif()
                if(ARG_SHADER_ENVIRONMENT)
                        message(FATAL_ERROR "Binary file SHADER_ENVIRONMENT is not supported")
                endif()

                set(binary_files TRUE)
                set(custom_target_prefix "custom_target_binary_files")

        elseif("${FILE_TYPE}" STREQUAL "VULKAN_SHADERS")

                if(NOT ARG_GLSL_COMPILER)
                        message(FATAL_ERROR "GLSL_COMPILER is not specified")
                endif()
                if(NOT ARG_SHADER_ENVIRONMENT)
                        message(FATAL_ERROR "SHADER_ENVIRONMENT is not specified")
                endif()
                if(NOT ARG_INCLUDE_EXTENSIONS)
                        message(FATAL_ERROR "INCLUDE_EXTENSIONS is not specified")
                endif()

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

        unset(include_directories)
        if(ARG_INCLUDE_DIRECTORIES)
                foreach(dir ${ARG_INCLUDE_DIRECTORIES})
                        if(NOT IS_DIRECTORY "${dir}")
                                message(FATAL_ERROR "Include directory ${dir} is not a directory")
                        endif()
                        list(APPEND include_directories "${dir}")
                endforeach()
        endif()

        #

        foreach(source ${ARG_SOURCE})

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

                                add_binary_file(
                                        INPUT_FILE ${input_file}
                                        OUTPUT_FILE ${output_file}
                                        CREATE_STR ${ARG_CREATE_STR})

                        elseif(vulkan_shaders)

                                add_shader_file(
                                        INPUT_FILE ${input_file}
                                        OUTPUT_FILE ${output_file}
                                        CREATE_STR ${ARG_CREATE_STR}
                                        GLSL_COMPILER ${ARG_GLSL_COMPILER}
                                        SHADER_ENVIRONMENT ${ARG_SHADER_ENVIRONMENT}
                                        INCLUDE_DIRECTORIES ${include_directories}
                                        INCLUDE_EXTENSIONS ${ARG_INCLUDE_EXTENSIONS}
                                        MACROS ${ARG_MACROS})

                        endif()

                endforeach()

        endforeach()

endfunction()
