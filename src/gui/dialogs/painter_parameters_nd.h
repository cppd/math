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

#include "painter_parameters.h"

#include "ui_painter_parameters_nd.h"

#include <array>
#include <optional>
#include <tuple>

namespace ns::gui::dialog
{
struct PainterParametersNd final
{
        int max_size;
};

class PainterParametersNdDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::PainterParametersNdDialog ui_;

        PainterParametersWidget* parameters_widget_;

        int min_screen_size_;
        int max_screen_size_;

        std::optional<std::tuple<PainterParameters, PainterParametersNd>>* const parameters_;

        PainterParametersNdDialog(
                int dimension,
                int max_thread_count,
                int screen_size,
                int min_screen_size,
                int max_screen_size,
                int samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int precision_index,
                const std::array<const char*, 2>& colors,
                int color_index,
                const std::array<const char*, 2>& integrators,
                int integrator_index,
                std::optional<std::tuple<PainterParameters, PainterParametersNd>>* parameters);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<std::tuple<PainterParameters, PainterParametersNd>> show(
                int dimension,
                int max_thread_count,
                int screen_size,
                int min_screen_size,
                int max_screen_size,
                int samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int precision_index,
                const std::array<const char*, 2>& colors,
                int color_index,
                const std::array<const char*, 2>& integrators,
                int integrator_index);
};
}
