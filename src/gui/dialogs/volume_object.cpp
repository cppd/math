/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/gui/com/support.h>

#include <QDialog>
#include <QString>

#include <algorithm>
#include <optional>
#include <string>

namespace ns::gui::dialogs
{
namespace
{
void check_parameters(
        const int dimension,
        const std::string& object_name,
        const int default_image_size,
        const int min_image_size,
        const int max_image_size)
{
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
}
}

VolumeObjectParametersDialog::VolumeObjectParametersDialog(
        const int dimension,
        const std::string& object_name,
        const int default_image_size,
        const int min_image_size,
        const int max_image_size,
        std::optional<VolumeObjectParameters>* const parameters)
        : QDialog(com::parent_for_dialog()),
          min_image_size_(min_image_size),
          max_image_size_(max_image_size),
          parameters_(parameters)
{
        check_parameters(dimension, object_name, default_image_size, min_image_size, max_image_size);

        ui_.setupUi(this);
        setWindowTitle("Create Object");

        ui_.label_space->setText(QString::fromStdString(space_name(dimension)));
        ui_.label_object->setText(QString::fromStdString(object_name));

        ui_.spin_box_image_size->setMinimum(min_image_size);
        ui_.spin_box_image_size->setMaximum(max_image_size);
        ui_.spin_box_image_size->setSingleStep(std::max(1, max_image_size / 1000));
        ui_.spin_box_image_size->setValue(default_image_size);

        com::set_dialog_size(this);
}

void VolumeObjectParametersDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        const int image_size = ui_.spin_box_image_size->value();
        if (!(image_size >= min_image_size_ && image_size <= max_image_size_))
        {
                const std::string msg =
                        "Error image size. It must be in the range [" + to_string(min_image_size_) + ", "
                        + to_string(max_image_size_) + "].";
                dialogs::message_critical(msg);
                return;
        }

        auto& parameters = parameters_->emplace();
        parameters.image_size = image_size;

        QDialog::done(r);
}

std::optional<VolumeObjectParameters> VolumeObjectParametersDialog::show(
        const int dimension,
        const std::string& object_name,
        const int default_image_size,
        const int min_image_size,
        const int max_image_size)
{
        std::optional<VolumeObjectParameters> parameters;

        const com::QtObjectInDynamicMemory w(new VolumeObjectParametersDialog(
                dimension, object_name, default_image_size, min_image_size, max_image_size, &parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        return parameters;
}
}
