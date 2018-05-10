/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include <QCommandLineParser>

constexpr const char NO_OBJECT_SELECTION_DIALOG_OPTION[] = "n";

namespace
{
std::string command_line_description_string()
{
        std::string s;

        s += "Usage:\n";
        s += "    program [option] [file]\n";
        s += "Argument:\n";
        s += "    file\n";
        s += "        The file to load\n";
        s += "Option:\n";
        s += "    -" + std::string(NO_OBJECT_SELECTION_DIALOG_OPTION) + "\n";
        s += "        Do not open object selection dialog.\n";
        s += "        Not used if there is no the file argument.";

        return s;
}
}

const std::string& command_line_description()
{
        static const std::string s = command_line_description_string();
        return s;
}

bool parse_command_line(std::function<void(const std::string& message)>&& error_message, std::string* file_to_load,
                        bool* use_object_selection_dialog)
{
        QCommandLineParser parser;

        const QCommandLineOption no_object_selection_dialog_option(NO_OBJECT_SELECTION_DIALOG_OPTION);

        if (!parser.addOption(no_object_selection_dialog_option))
        {
                error_message("Failed to add a command line option.");
                return false;
        }

        if (!parser.parse(QCoreApplication::arguments()))
        {
                error_message(parser.errorText().toStdString());
                return false;
        }

        const QStringList positional_arguments = parser.positionalArguments();

        if (positional_arguments.size() > 1)
        {
                error_message("Too many file name arguments.");
                return false;
        }

        if (positional_arguments.size() == 0)
        {
                return false;
        }

        *file_to_load = positional_arguments.value(0).toStdString();
        *use_object_selection_dialog = !parser.isSet(no_object_selection_dialog_option);

        return true;
}
