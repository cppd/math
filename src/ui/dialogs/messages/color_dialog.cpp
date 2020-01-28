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

#include "color_dialog.h"

#include "ui/support/support.h"

#include <QColorDialog>

namespace dialog
{
void color_dialog(
        QWidget* parent,
        const std::string& title,
        const QColor& current_color,
        const std::function<void(const QColor& c)>& f)
{
        QtObjectInDynamicMemory<QColorDialog> dialog(parent);

        dialog->setCurrentColor(current_color);
        dialog->setWindowTitle(title.c_str());
        dialog->setOptions(QColorDialog::NoButtons | QColorDialog::DontUseNativeDialog);

        QObject::connect(dialog, &QColorDialog::currentColorChanged, [&](const QColor& color) {
                if (color.isValid())
                {
                        f(color);
                }
        });

        dialog->exec();
}
}
