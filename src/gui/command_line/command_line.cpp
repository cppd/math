/*
Copyright (C) 2017-2020 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "command_line.h"

#include <src/com/error.h>

#include <QCommandLineParser>

constexpr const char* NO_OBJECT_SELECTION_OPTION = "n";

#if defined(OPENGL_FOUND)
constexpr const char* VULKAN_OPTION = "vulkan";
constexpr const char* OPENGL_OPTION = "opengl";
#endif

namespace
{
std::string command_line_description_string()
{
        std::string s;

        s += "Usage:\n";

        s += "    program";
#if defined(OPENGL_FOUND)
        s += " [--" + std::string(VULKAN_OPTION) + "|--" + std::string(OPENGL_OPTION) + "]";
#endif
        s += " [[-" + std::string(NO_OBJECT_SELECTION_OPTION) + "] FILE]";
        s += '\n';

        s += "Description:\n";

        s += "    FILE\n";
        s += "        the file to load\n";
        s += "    -" + std::string(NO_OBJECT_SELECTION_OPTION) + "\n";
        s += "        do not open object selection dialog\n";
#if defined(OPENGL_FOUND)
        s += "    --" + std::string(VULKAN_OPTION) + "\n";
        s += "        use Vulkan API\n";
        s += "    --" + std::string(OPENGL_OPTION) + "\n";
        s += "        use OpenGL API\n";
#endif

        return s;
}
}

const std::string& command_line_description()
{
        static const std::string s = command_line_description_string();
        return s;
}

CommandLineOptions command_line_options()
{
        QCommandLineParser parser;

        QCommandLineOption no_object_selection_option(NO_OBJECT_SELECTION_OPTION);
#if defined(OPENGL_FOUND)
        QCommandLineOption vulkan_option(VULKAN_OPTION);
        QCommandLineOption opengl_option(OPENGL_OPTION);
        if (!parser.addOptions({no_object_selection_option, vulkan_option, opengl_option}))
        {
                error("Failed to add command line options");
        }
#else
        if (!parser.addOptions({no_object_selection_option}))
        {
                error("Failed to add command line options");
        }
#endif

        if (!parser.parse(QCoreApplication::arguments()))
        {
                error(parser.errorText().toStdString());
        }

        QStringList positional_arguments = parser.positionalArguments();

        //

        CommandLineOptions options;

        //

        if (positional_arguments.size() > 1)
        {
                error("Too many file name arguments");
        }
        else if (positional_arguments.size() == 1)
        {
                if (positional_arguments.value(0).size() < 1)
                {
                        error("Empty the file name argument");
                }

                options.file_name = positional_arguments.value(0).toStdString();
                options.no_object_selection_dialog = parser.isSet(no_object_selection_option);
        }
        else
        {
                if (parser.isSet(no_object_selection_option))
                {
                        error("Object selection dialog option without the file name argument");
                }

                options.file_name = "";
                options.no_object_selection_dialog = false;
        }

        //

#if defined(OPENGL_FOUND)
        if (parser.isSet(vulkan_option) && parser.isSet(opengl_option))
        {
                error(std::string("Specified mutually exclusive options ") + VULKAN_OPTION + " and " + OPENGL_OPTION);
        }
        else if (parser.isSet(vulkan_option))
        {
                options.graphics_and_compute_api = GraphicsAndComputeAPI::Vulkan;
        }
        else if (parser.isSet(opengl_option))
        {
                options.graphics_and_compute_api = GraphicsAndComputeAPI::OpenGL;
        }
#endif
        //

        return options;
}
