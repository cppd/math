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

#include "application_help.h"

#include "application/application_name.h"

#include <QMessageBox>
#include <string>

namespace
{
std::string message()
{
        std::string message;

        message +=
                "Move: left mouse button.\n\n"
                "Rotate: right mouse button.\n\n"
                "Zoom: mouse wheel.\n\n"
                "Toggle fullscreen: F11.";

        return message;
}

std::string title()
{
        return std::string(APPLICATION_NAME) + " Help";
}
}

void application_help(QWidget* parent)
{
        static const QString m = message().c_str();
        static const QString t = title().c_str();
        QMessageBox::about(parent, t, m);
}
