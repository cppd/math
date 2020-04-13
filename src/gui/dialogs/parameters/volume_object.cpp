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

#include "volume_object.h"

#include "../../support/support.h"
#include "../messages/message_box.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

#include <QPointer>

namespace volume_object_parameters_implementation
{
VolumeObjectParameters::VolumeObjectParameters(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Create Object");
}

bool VolumeObjectParameters::show(
        int dimension,
        const std::string& volume_object_name,
        int default_image_size,
        int min_image_size,
        int max_image_size,
        int* image_size)
{
        if (!(dimension >= 2))
        {
                error("Error dimension parameter: " + to_string(dimension));
        }
        if (volume_object_name.empty())
        {
                error("No volume object name parameter");
        }
        if (!(1 <= min_image_size && min_image_size <= default_image_size && default_image_size <= max_image_size))
        {
                error("Error image size parameters: min = " + to_string(min_image_size) +
                      ", max = " + to_string(max_image_size) + ", default = " + to_string(default_image_size));
        }

        m_min_image_size = min_image_size;
        m_max_image_size = max_image_size;

        ui.label_space->setText(space_name(dimension).c_str());
        ui.label_object->setText(volume_object_name.c_str());

        ui.spinBox_image_size->setMinimum(min_image_size);
        ui.spinBox_image_size->setMaximum(max_image_size);
        ui.spinBox_image_size->setValue(default_image_size);
        ui.spinBox_image_size->setSingleStep(std::max(1, max_image_size / 1000));

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
        {
                return false;
        }

        *image_size = m_image_size;

        return true;
}

void VolumeObjectParameters::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        m_image_size = ui.spinBox_image_size->value();
        if (!(m_image_size >= m_min_image_size && m_image_size <= m_max_image_size))
        {
                std::string msg = "Error image size. It must be in the range [" + to_string(m_min_image_size) + ", " +
                                  to_string(m_max_image_size) + "].";
                dialog::message_critical(this, msg);
                return;
        }

        QDialog::done(r);
}
}

namespace dialog
{
bool volume_object_parameters(
        QWidget* parent,
        int dimension,
        const std::string& volume_object_name,
        int default_image_size,
        int min_image_size,
        int max_image_size,
        int* image_size)
{
        QtObjectInDynamicMemory<volume_object_parameters_implementation::VolumeObjectParameters> w(parent);
        return w->show(dimension, volume_object_name, default_image_size, min_image_size, max_image_size, image_size);
}
}
