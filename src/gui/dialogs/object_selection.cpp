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

#include <mutex>

namespace gui::dialog
{
namespace
{
std::mutex g_mutex;

bool g_bound_cocone = true;
bool g_cocone = true;
bool g_convex_hull = true;
bool g_mst = true;

ObjectSelection read_current()
{
        ObjectSelection current;
        std::lock_guard lg(g_mutex);
        if (g_bound_cocone)
        {
                current.types.insert(ObjectSelection::Type::BoundCocone);
        }
        if (g_cocone)
        {
                current.types.insert(ObjectSelection::Type::Cocone);
        }
        if (g_convex_hull)
        {
                current.types.insert(ObjectSelection::Type::ConvexHull);
        }
        if (g_mst)
        {
                current.types.insert(ObjectSelection::Type::Mst);
        }
        return current;
}

void write_current(const ObjectSelection& current)
{
        std::lock_guard lg(g_mutex);
        g_bound_cocone = current.types.contains(ObjectSelection::Type::BoundCocone);
        g_cocone = current.types.contains(ObjectSelection::Type::Cocone);
        g_convex_hull = current.types.contains(ObjectSelection::Type::ConvexHull);
        g_mst = current.types.contains(ObjectSelection::Type::Mst);
}
}

ObjectSelectionDialog::ObjectSelectionDialog(const ObjectSelection& input, std::optional<ObjectSelection>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Object Selection");

        m_boxes.push_back(ui.checkBox_model_convex_hull);
        m_boxes.push_back(ui.checkBox_model_minumum_spanning_tree);
        m_boxes.push_back(ui.checkBox_cocone);
        m_boxes.push_back(ui.checkBox_bound_cocone);

        connect(ui.pushButton_set_all, &QPushButton::clicked, this,
                [this]()
                {
                        set_all(true);
                });
        connect(ui.pushButton_clear_all, &QPushButton::clicked, this,
                [this]()
                {
                        set_all(false);
                });

        ui.checkBox_bound_cocone->setChecked(input.types.contains(ObjectSelection::Type::BoundCocone));
        ui.checkBox_cocone->setChecked(input.types.contains(ObjectSelection::Type::Cocone));
        ui.checkBox_model_convex_hull->setChecked(input.types.contains(ObjectSelection::Type::ConvexHull));
        ui.checkBox_model_minumum_spanning_tree->setChecked(input.types.contains(ObjectSelection::Type::Mst));
}

void ObjectSelectionDialog::set_all(bool checked)
{
        for (QCheckBox* c : m_boxes)
        {
                c->setChecked(checked);
        }
}

void ObjectSelectionDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        m_parameters.emplace();

        if (ui.checkBox_bound_cocone->isChecked())
        {
                m_parameters->types.insert(ObjectSelection::Type::BoundCocone);
        }
        if (ui.checkBox_cocone->isChecked())
        {
                m_parameters->types.insert(ObjectSelection::Type::Cocone);
        }
        if (ui.checkBox_model_convex_hull->isChecked())
        {
                m_parameters->types.insert(ObjectSelection::Type::ConvexHull);
        }
        if (ui.checkBox_model_minumum_spanning_tree->isChecked())
        {
                m_parameters->types.insert(ObjectSelection::Type::Mst);
        }

        QDialog::done(r);
}

std::optional<ObjectSelection> ObjectSelectionDialog::show()
{
        std::optional<ObjectSelection> parameters;

        QtObjectInDynamicMemory w(new ObjectSelectionDialog(read_current(), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        write_current(*parameters);

        return parameters;
}

ObjectSelection ObjectSelectionDialog::current()
{
        return read_current();
}
}
