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

#include "facet_object.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace ns::gui::dialog
{
FacetObjectParametersDialog::FacetObjectParametersDialog(
        int dimension,
        const std::string& object_name,
        int default_facet_count,
        int min_facet_count,
        int max_facet_count,
        std::optional<FacetObjectParameters>& parameters)
        : QDialog(parent_for_dialog()),
          m_min_facet_count(min_facet_count),
          m_max_facet_count(max_facet_count),
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
                error("No facet object name parameter");
        }
        if (!(1 <= min_facet_count))
        {
                error("Minumum facet count " + to_string(min_facet_count) + " must be greater than or equal to 1");
        }
        if (!(min_facet_count <= max_facet_count))
        {
                error("Maximum facet count " + to_string(max_facet_count)
                      + " must be greater than or equal to minimum facet count " + to_string(min_facet_count));
        }
        if (!(min_facet_count <= default_facet_count && default_facet_count <= max_facet_count))
        {
                error("Initial facet count must be in the range [" + to_string(min_facet_count) + ", "
                      + to_string(max_facet_count) + "]");
        }

        ui.label_space->setText(QString::fromStdString(space_name(dimension)));
        ui.label_object->setText(QString::fromStdString(object_name));

        ui.spinBox_facet_count->setMinimum(min_facet_count);
        ui.spinBox_facet_count->setMaximum(max_facet_count);
        ui.spinBox_facet_count->setValue(default_facet_count);
        ui.spinBox_facet_count->setSingleStep(std::max(1, max_facet_count / 1000));
}

void FacetObjectParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int facet_count = ui.spinBox_facet_count->value();
        if (!(facet_count >= m_min_facet_count && facet_count <= m_max_facet_count))
        {
                std::string msg = "Facet count must be in the range [" + to_string(m_min_facet_count) + ", "
                                  + to_string(m_max_facet_count) + "].";
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();
        m_parameters->facet_count = facet_count;

        QDialog::done(r);
}

std::optional<FacetObjectParameters> FacetObjectParametersDialog::show(
        int dimension,
        const std::string& object_name,
        int default_facet_count,
        int min_facet_count,
        int max_facet_count)
{
        std::optional<FacetObjectParameters> parameters;

        QtObjectInDynamicMemory w(new FacetObjectParametersDialog(
                dimension, object_name, default_facet_count, min_facet_count, max_facet_count, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
