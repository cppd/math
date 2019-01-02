/*
Copyright (C) 2017-2019 Topological Manifold

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

#pragma once

#include "ui_object_selection.h"

#include <vector>

namespace object_selection_implementation
{
class ObjectSelection final : public QDialog
{
        Q_OBJECT

public:
        explicit ObjectSelection(QWidget* parent = nullptr);

        [[nodiscard]] bool show(bool* model_convex_hull, bool* model_minumum_spanning_tree, bool* cocone,
                                bool* cocone_convex_hull, bool* bound_cocone, bool* bound_cocone_convex_hull);

private slots:

        void on_pushButton_set_all_clicked();
        void on_pushButton_clear_all_clicked();

private:
        Ui::ObjectSelection ui;

        std::vector<QCheckBox*> m_boxes;
};
}

namespace dialog
{
[[nodiscard]] bool object_selection(QWidget* parent, bool* model_convex_hull, bool* model_minumum_spanning_tree, bool* cocone,
                                    bool* cocone_convex_hull, bool* bound_cocone, bool* bound_cocone_convex_hull);
}
