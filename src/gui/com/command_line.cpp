/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/com/file/path.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <string>

namespace ns::gui
{
namespace
{
constexpr const char* NO_OBJECT_SELECTION_OPTION = "n";
}

std::string command_line_description()
{
        std::string s;

        s += "Usage:\n";

        s += "    program";

        s += " [[-" + std::string(NO_OBJECT_SELECTION_OPTION) + "] FILE]";
        s += '\n';

        s += "Description:\n";

        s += "    FILE\n";
        s += "        the file to load\n";
        s += "    -" + std::string(NO_OBJECT_SELECTION_OPTION) + "\n";
        s += "        do not open object selection dialog\n";

        return s;
}

CommandLineOptions command_line_options()
{
        QCommandLineParser parser;

        const QCommandLineOption no_object_selection_option(NO_OBJECT_SELECTION_OPTION);

        if (!parser.addOptions({no_object_selection_option}))
        {
                error("Failed to add command line options");
        }

        if (!parser.parse(QCoreApplication::arguments()))
        {
                error(parser.errorText().toStdString());
        }

        const QStringList positional_arguments = parser.positionalArguments();

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

                options.file_name = path_from_utf8(positional_arguments.value(0).toStdString());
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

        return options;
}
}
