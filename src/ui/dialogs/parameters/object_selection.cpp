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

#include "object_selection.h"

ObjectSelection::ObjectSelection(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Object Selection");

        m_boxes.push_back(ui.checkBox_model_convex_hull);
        m_boxes.push_back(ui.checkBox_model_minumum_spanning_tree);
        m_boxes.push_back(ui.checkBox_cocone);
        m_boxes.push_back(ui.checkBox_cocone_convex_hull);
        m_boxes.push_back(ui.checkBox_bound_cocone);
        m_boxes.push_back(ui.checkBox_bound_cocone_convex_hull);
}

bool ObjectSelection::show(bool* model_convex_hull, bool* model_minumum_spanning_tree, bool* cocone, bool* cocone_convex_hull,
                           bool* bound_cocone, bool* bound_cocone_convex_hull)
{
        ui.checkBox_model_convex_hull->setChecked(*model_convex_hull);
        ui.checkBox_model_minumum_spanning_tree->setChecked(*model_minumum_spanning_tree);
        ui.checkBox_cocone->setChecked(*cocone);
        ui.checkBox_cocone_convex_hull->setChecked(*cocone_convex_hull);
        ui.checkBox_bound_cocone->setChecked(*bound_cocone);
        ui.checkBox_bound_cocone_convex_hull->setChecked(*bound_cocone_convex_hull);

        if (!this->exec())
        {
                return false;
        }

        *model_convex_hull = ui.checkBox_model_convex_hull->isChecked();
        *model_minumum_spanning_tree = ui.checkBox_model_minumum_spanning_tree->isChecked();
        *cocone = ui.checkBox_cocone->isChecked();
        *cocone_convex_hull = ui.checkBox_cocone_convex_hull->isChecked();
        *bound_cocone = ui.checkBox_bound_cocone->isChecked();
        *bound_cocone_convex_hull = ui.checkBox_bound_cocone_convex_hull->isChecked();

        return true;
}

void ObjectSelection::on_pushButton_set_all_clicked()
{
        for (QCheckBox* c : m_boxes)
        {
                c->setChecked(true);
        }
}

void ObjectSelection::on_pushButton_clear_all_clicked()
{
        for (QCheckBox* c : m_boxes)
        {
                c->setChecked(false);
        }
}
