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

#include "volume_object.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace ns::gui::dialog
{
VolumeObjectParametersDialog::VolumeObjectParametersDialog(
        int dimension,
        const std::string& object_name,
        int default_image_size,
        int min_image_size,
        int max_image_size,
        std::optional<VolumeObjectParameters>& parameters)
        : QDialog(parent_for_dialog()),
          min_image_size_(min_image_size),
          max_image_size_(max_image_size),
          parameters_(parameters)
{
        ui_.setupUi(this);
        setWindowTitle("Create Object");

        if (!(dimension >= 2))
        {
                error("Error dimension parameter: " + to_string(dimension));
        }
        if (object_name.empty())
        {
                error("No volume object name parameter");
        }
        if (!(1 <= min_image_size))
        {
                error("Minumum image size " + to_string(min_image_size) + " must be greater than or equal to 1");
        }
        if (!(min_image_size <= max_image_size))
        {
                error("Maximum image size " + to_string(max_image_size)
                      + " must be greater than or equal to minimum image size " + to_string(min_image_size));
        }
        if (!(min_image_size <= default_image_size && default_image_size <= max_image_size))
        {
                error("Initial image size must be in the range [" + to_string(min_image_size) + ", "
                      + to_string(max_image_size) + "]");
        }

        ui_.label_space->setText(QString::fromStdString(space_name(dimension)));
        ui_.label_object->setText(QString::fromStdString(object_name));

        ui_.spinBox_image_size->setMinimum(min_image_size);
        ui_.spinBox_image_size->setMaximum(max_image_size);
        ui_.spinBox_image_size->setValue(default_image_size);
        ui_.spinBox_image_size->setSingleStep(std::max(1, max_image_size / 1000));
}

void VolumeObjectParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int image_size = ui_.spinBox_image_size->value();
        if (!(image_size >= min_image_size_ && image_size <= max_image_size_))
        {
                std::string msg = "Error image size. It must be in the range [" + to_string(min_image_size_) + ", "
                                  + to_string(max_image_size_) + "].";
                dialog::message_critical(msg);
                return;
        }

        parameters_.emplace();
        parameters_->image_size = image_size;

        QDialog::done(r);
}

std::optional<VolumeObjectParameters> VolumeObjectParametersDialog::show(
        int dimension,
        const std::string& object_name,
        int default_image_size,
        int min_image_size,
        int max_image_size)
{
        std::optional<VolumeObjectParameters> parameters;

        QtObjectInDynamicMemory w(new VolumeObjectParametersDialog(
                dimension, object_name, default_image_size, min_image_size, max_image_size, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
