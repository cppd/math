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

#include "point_object.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace ns::gui::dialog
{
PointObjectParametersDialog::PointObjectParametersDialog(
        int dimension,
        const std::string& object_name,
        int default_point_count,
        int min_point_count,
        int max_point_count,
        std::optional<PointObjectParameters>& parameters)
        : QDialog(parent_for_dialog()),
          m_min_point_count(min_point_count),
          m_max_point_count(max_point_count),
          m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Create Object");

        if (!(dimension >= 2))
        {
                error("Dimension " + to_string(dimension) + " must be greater than or equal to 2");
        }
        if (object_name.empty())
        {
                error("No point object name parameter");
        }
        if (!(1 <= min_point_count))
        {
                error("Minumum point count " + to_string(min_point_count) + " must be greater than or equal to 1");
        }
        if (!(min_point_count <= max_point_count))
        {
                error("Maximum point count " + to_string(max_point_count)
                      + " must be greater than or equal to minimum point count " + to_string(min_point_count));
        }
        if (!(min_point_count <= default_point_count && default_point_count <= max_point_count))
        {
                error("Initial point count must be in the range [" + to_string(min_point_count) + ", "
                      + to_string(max_point_count) + "]");
        }

        ui.label_space->setText(QString::fromStdString(space_name(dimension)));
        ui.label_object->setText(QString::fromStdString(object_name));

        ui.spinBox_point_count->setMinimum(min_point_count);
        ui.spinBox_point_count->setMaximum(max_point_count);
        ui.spinBox_point_count->setValue(default_point_count);
        ui.spinBox_point_count->setSingleStep(std::max(1, max_point_count / 1000));
}

void PointObjectParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int point_count = ui.spinBox_point_count->value();
        if (!(point_count >= m_min_point_count && point_count <= m_max_point_count))
        {
                std::string msg = "Point count must be in the range [" + to_string(m_min_point_count) + ", "
                                  + to_string(m_max_point_count) + "].";
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();
        m_parameters->point_count = point_count;

        QDialog::done(r);
}

std::optional<PointObjectParameters> PointObjectParametersDialog::show(
        int dimension,
        const std::string& object_name,
        int default_point_count,
        int min_point_count,
        int max_point_count)
{
        std::optional<PointObjectParameters> parameters;

        QtObjectInDynamicMemory w(new PointObjectParametersDialog(
                dimension, object_name, default_point_count, min_point_count, max_point_count, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
