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

#include "object_selection.h"

#include "../com/support.h"

#include <QPointer>

namespace dialog
{
namespace object_selection_implementation
{
namespace
{
bool g_bound_cocone = true;
bool g_cocone = true;
bool g_convex_hull = true;
bool g_mst = true;
}

ObjectSelection::ObjectSelection(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Object Selection");

        m_boxes.push_back(ui.checkBox_model_convex_hull);
        m_boxes.push_back(ui.checkBox_model_minumum_spanning_tree);
        m_boxes.push_back(ui.checkBox_cocone);
        m_boxes.push_back(ui.checkBox_bound_cocone);
}

bool ObjectSelection::show(bool* bound_cocone, bool* cocone, bool* convex_hull, bool* mst)
{
        ui.checkBox_bound_cocone->setChecked(*bound_cocone);
        ui.checkBox_cocone->setChecked(*cocone);
        ui.checkBox_model_convex_hull->setChecked(*convex_hull);
        ui.checkBox_model_minumum_spanning_tree->setChecked(*mst);

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
        {
                return false;
        }

        *bound_cocone = ui.checkBox_bound_cocone->isChecked();
        *cocone = ui.checkBox_cocone->isChecked();
        *convex_hull = ui.checkBox_model_convex_hull->isChecked();
        *mst = ui.checkBox_model_minumum_spanning_tree->isChecked();

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
}

bool object_selection(QWidget* parent, std::unordered_set<ComputationType>* objects_to_load)
{
        ASSERT(objects_to_load);

        namespace impl = object_selection_implementation;

        QtObjectInDynamicMemory<impl::ObjectSelection> w(parent);

        if (!w->show(&impl::g_bound_cocone, &impl::g_cocone, &impl::g_convex_hull, &impl::g_mst))
        {
                return false;
        }

        objects_to_load->clear();

        if (impl::g_bound_cocone)
        {
                objects_to_load->insert(ComputationType::BoundCocone);
        }

        if (impl::g_cocone)
        {
                objects_to_load->insert(ComputationType::Cocone);
        }

        if (impl::g_convex_hull)
        {
                objects_to_load->insert(ComputationType::ConvexHull);
        }

        if (impl::g_mst)
        {
                objects_to_load->insert(ComputationType::Mst);
        }

        return true;
}

std::unordered_set<ComputationType> object_selection_current()
{
        std::unordered_set<ComputationType> objects_to_load;

        objects_to_load.insert(ComputationType::BoundCocone);
        objects_to_load.insert(ComputationType::Cocone);
        objects_to_load.insert(ComputationType::ConvexHull);
        objects_to_load.insert(ComputationType::Mst);

        return objects_to_load;
}
}
