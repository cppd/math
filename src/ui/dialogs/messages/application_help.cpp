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

#include "application_help.h"

#include "application/name.h"
#include "ui/support/support.h"

#include <QMessageBox>
#include <string>

namespace
{
std::string message()
{
        return "Move: left mouse button.\n\n"
               "Rotate: right mouse button.\n\n"
               "Zoom: mouse wheel.\n\n"
               "Toggle fullscreen: F11.";
}

std::string title()
{
        return std::string(APPLICATION_NAME) + " Help";
}
}

namespace dialog
{
void application_help(QWidget* parent)
{
        static const QString t = title().c_str();
        static const QString m = message().c_str();

        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::NoIcon, t, m, QMessageBox::Ok, parent);
        w->exec();
}
}
