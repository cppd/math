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

#pragma once

#include "painter_parameters.h"

#include "ui_painter_parameters_3d.h"

#include <QDialog>
#include <QObject>

#include <array>
#include <optional>
#include <tuple>

namespace ns::gui::dialogs
{
struct PainterParameters3d final
{
        int width;
        int height;
};

class PainterParameters3dDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::PainterParameters3dDialog ui_;

        PainterParametersWidget* parameters_widget_;

        double aspect_ratio_;
        int max_width_;
        int min_width_;
        int max_height_;
        int min_height_;

        std::optional<std::tuple<PainterParameters, PainterParameters3d>>* const parameters_;

        PainterParameters3dDialog(
                int max_thread_count,
                int width,
                int height,
                int max_screen_size,
                int samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int precision_index,
                const std::array<const char*, 2>& colors,
                int color_index,
                const std::array<const char*, 2>& integrators,
                int integrator_index,
                std::optional<std::tuple<PainterParameters, PainterParameters3d>>* parameters);

        void on_width_value_changed(int);
        void on_height_value_changed(int);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<std::tuple<PainterParameters, PainterParameters3d>> show(
                int max_thread_count,
                int width,
                int height,
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
