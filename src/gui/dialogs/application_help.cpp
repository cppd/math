/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/gui/com/support.h>
#include <src/settings/name.h>

#include <QMessageBox>
#include <QString>

namespace ns::gui::dialogs
{
namespace
{
QString message()
{
        return "Move: left mouse button.\n\n"
               "Rotate: right mouse button.\n\n"
               "Zoom: mouse wheel.";
}

QString title()
{
        return QString(settings::APPLICATION_NAME) + " Help";
}
}

void application_help()
{
        const QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::NoIcon, title(), message(), QMessageBox::Ok, parent_for_dialog());

        w->exec();
}
}
