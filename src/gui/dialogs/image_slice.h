/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "ui_image_slice.h"

#include <QDialog>
#include <QObject>

#include <optional>
#include <vector>

namespace ns::gui::dialog
{
struct ImageSliceParameters final
{
        std::vector<std::optional<int>> slices;
};

class ImageSliceDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::ImageSliceDialog ui_;

        const int slice_dimension_;

        std::vector<std::optional<int>> slices_;

        std::optional<ImageSliceParameters>* const parameters_;

        ImageSliceDialog(
                const std::vector<int>& size,
                int slice_dimension,
                std::optional<ImageSliceParameters>* parameters);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<ImageSliceParameters> show(
                const std::vector<int>& size,
                int slice_dimension);
};
}
