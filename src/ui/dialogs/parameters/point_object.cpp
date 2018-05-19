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

#include "point_object.h"

#include "com/error.h"
#include "com/names.h"
#include "com/print.h"
#include "ui/dialogs/messages/message_box.h"
#include "ui/support/support.h"

#include <QPointer>

namespace point_object_parameters_implementation
{
PointObjectParameters::PointObjectParameters(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Create Object");
}

bool PointObjectParameters::show(int dimension, const std::string& point_object_name, int default_point_count,
                                 int min_point_count, int max_point_count, int* point_count)
{
        if (!(dimension >= 2))
        {
                error("Error dimension parameter: " + to_string(dimension));
        }
        if (point_object_name.empty())
        {
                error("No point object name parameter");
        }
        if (!(1 <= min_point_count && min_point_count <= default_point_count && default_point_count <= max_point_count))
        {
                error("Error point count parameters: min = " + to_string(min_point_count) +
                      ", max = " + to_string(max_point_count) + ", default = " + to_string(default_point_count));
        }

        m_min_point_count = min_point_count;
        m_max_point_count = max_point_count;

        ui.label_space->setText(space_name(dimension).c_str());
        ui.label_object->setText(point_object_name.c_str());

        ui.spinBox_point_count->setMinimum(min_point_count);
        ui.spinBox_point_count->setMaximum(max_point_count);
        ui.spinBox_point_count->setValue(default_point_count);
        ui.spinBox_point_count->setSingleStep(std::max(1, max_point_count / 1000));

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
        {
                return false;
        }

        *point_count = m_point_count;

        return true;
}

void PointObjectParameters::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        m_point_count = ui.spinBox_point_count->value();
        if (!(m_point_count >= 1 && m_point_count <= m_max_point_count))
        {
                std::string msg = "Error point count. It must be in the range [1, " + to_string(m_max_point_count) + "].";
                message_critical(this, msg.c_str());
                return;
        }

        QDialog::done(r);
}
}

bool point_object_parameters(QWidget* parent, int dimension, const std::string& point_object_name, int default_point_count,
                             int min_point_count, int max_point_count, int* point_count)
{
        QtObjectInDynamicMemory<point_object_parameters_implementation::PointObjectParameters> w(parent);
        return w->show(dimension, point_object_name, default_point_count, min_point_count, max_point_count, point_count);
}
