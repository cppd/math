/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "color_dialog.h"

#include <src/gui/com/support.h>

#include <QColor>
#include <QColorDialog>
#include <QObject>

#include <functional>
#include <string>

namespace ns::gui::dialogs
{
void color_dialog(const std::string& title, const QColor& current_color, const std::function<void(const QColor& c)>& f)
{
        const com::QtObjectInDynamicMemory<QColorDialog> dialog(com::parent_for_dialog());

        dialog->setWindowTitle(QString::fromStdString(title));
        dialog->setOptions(QColorDialog::NoButtons | QColorDialog::DontUseNativeDialog);

        dialog->setCurrentColor(current_color);

        QObject::connect(
                dialog, &QColorDialog::currentColorChanged,
                [&](const QColor& color)
                {
                        if (color.isValid())
                        {
                                f(color);
                        }
                });

        dialog->exec();
}
}
