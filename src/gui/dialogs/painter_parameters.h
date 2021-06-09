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

#pragma once

#include "ui_painter_parameters.h"

#include <array>
#include <string>

namespace ns::gui::dialog
{
class PainterParametersWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::PainterParametersWidget ui;

        int m_max_thread_count;
        int m_max_samples_per_pixel;

public:
        PainterParametersWidget(
                QWidget* parent,
                int max_thread_count,
                int default_samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int default_precision_index,
                const std::array<const char*, 2>& colors,
                int default_color_index);

        [[nodiscard]] bool check();

        int thread_count();
        int samples_per_pixel();
        bool flat_facets();
        bool cornell_box();
        int precision_index();
        int color_index();
};
}
