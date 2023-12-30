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

#include "ui_painter_parameters.h"

#include <QObject>
#include <QWidget>

#include <array>

namespace ns::gui::dialog
{
struct PainterParameters final
{
        int thread_count;
        int samples_per_pixel;
        bool flat_shading;
        bool cornell_box;
        int precision_index;
        int color_index;
        int integrator_index;
};

class PainterParametersWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::PainterParametersWidget ui_;

        int max_thread_count_;
        int max_samples_per_pixel_;

public:
        PainterParametersWidget(
                QWidget* parent,
                int max_thread_count,
                int samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int precision_index,
                const std::array<const char*, 2>& colors,
                int color_index,
                const std::array<const char*, 2>& integrators,
                int integrator_index);

        [[nodiscard]] bool check();
        [[nodiscard]] PainterParameters parameters();
};
}
