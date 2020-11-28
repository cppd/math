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

#include <optional>
#include <string>

namespace gui::dialog
{
struct VolumeObjectParameters final
{
        int image_size;
};

class VolumeObjectParametersDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::VolumeObjectParametersDialog ui;

        const int m_min_image_size;
        const int m_max_image_size;

        std::optional<VolumeObjectParameters>& m_parameters;

        VolumeObjectParametersDialog(
                int dimension,
                const std::string& volume_object_name,
                int default_image_size,
                int min_image_size,
                int max_image_size,
                std::optional<VolumeObjectParameters>& parameters);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<VolumeObjectParameters> show(
                int dimension,
                const std::string& volume_object_name,
                int default_image_size,
                int min_image_size,
                int max_image_size);
};
}
