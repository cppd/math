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

#pragma once

#include "ui_volume_object.h"

#include <string>

namespace gui::dialog
{
namespace volume_object_parameters_implementation
{
class VolumeObjectParameters final : public QDialog
{
        Q_OBJECT

public:
        explicit VolumeObjectParameters(QWidget* parent = nullptr);

        [[nodiscard]] bool show(
                int dimension,
                const std::string& point_object_name,
                int default_image_size,
                int min_image_size,
                int max_image_size,
                int* image_size);

private:
        int m_min_image_size;
        int m_max_image_size;
        int m_image_size;

        Ui::VolumeObjectParameters ui;

        void done(int r) override;
};
}

[[nodiscard]] bool volume_object_parameters(
        int dimension,
        const std::string& volume_object_name,
        int default_image_size,
        int min_image_size,
        int max_image_size,
        int* image_size);
}
