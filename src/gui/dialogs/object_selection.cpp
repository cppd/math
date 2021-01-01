/*
Copyright (C) 2017-2021 Topological Manifold

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

namespace ns::gui::dialog
{
namespace
{
std::mutex g_mutex;
ObjectSelectionParameters g_parameters{.bound_cocone = true, .cocone = true, .convex_hull = true, .mst = true};

ObjectSelectionParameters read_current()
{
        std::lock_guard lg(g_mutex);
        return g_parameters;
}

void write_current(const ObjectSelectionParameters& parameters)
{
        std::lock_guard lg(g_mutex);
        g_parameters = parameters;
}
}

ObjectSelectionParametersDialog::ObjectSelectionParametersDialog(
        const ObjectSelectionParameters& input,
        std::optional<ObjectSelectionParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Object Selection");

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

        ui.checkBox_bound_cocone->setChecked(input.bound_cocone);
        ui.checkBox_cocone->setChecked(input.cocone);
        ui.checkBox_convex_hull->setChecked(input.convex_hull);
        ui.checkBox_minumum_spanning_tree->setChecked(input.mst);
}

void ObjectSelectionParametersDialog::set_all(bool checked)
{
        ui.checkBox_convex_hull->setChecked(checked);
        ui.checkBox_minumum_spanning_tree->setChecked(checked);
        ui.checkBox_cocone->setChecked(checked);
        ui.checkBox_bound_cocone->setChecked(checked);
}

void ObjectSelectionParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        m_parameters.emplace();
        m_parameters->bound_cocone = ui.checkBox_bound_cocone->isChecked();
        m_parameters->cocone = ui.checkBox_cocone->isChecked();
        m_parameters->convex_hull = ui.checkBox_convex_hull->isChecked();
        m_parameters->mst = ui.checkBox_minumum_spanning_tree->isChecked();

        QDialog::done(r);
}

std::optional<ObjectSelectionParameters> ObjectSelectionParametersDialog::show()
{
        std::optional<ObjectSelectionParameters> parameters;

        QtObjectInDynamicMemory w(new ObjectSelectionParametersDialog(read_current(), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        write_current(*parameters);

        return parameters;
}

ObjectSelectionParameters ObjectSelectionParametersDialog::current()
{
        return read_current();
}
}
