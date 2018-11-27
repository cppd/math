/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "ui_painter_3d.h"

namespace painter_parameters_for_3d_implementation
{
class PainterParametersFor3d final : public QDialog
{
        Q_OBJECT

public:
        explicit PainterParametersFor3d(QWidget* parent = nullptr);

        [[nodiscard]] bool show(int max_thread_count, int width, int height, int max_screen_size, int default_samples_per_pixel,
                                int max_samples_per_pixel, int* thread_count, int* paint_width, int* paint_height,
                                int* samples_per_pixel, bool* flat_facets, bool* cornell_box);

private slots:

        void width_value_changed(int);
        void height_value_changed(int);

private:
        int m_width;
        int m_height;
        int m_min_width;
        int m_max_width;
        int m_min_height;
        int m_max_height;
        double m_aspect_ratio;
        int m_max_thread_count;
        int m_max_samples_per_pixel;

        int m_thread_count;
        double m_size_coef;
        int m_samples_per_pixel;
        bool m_flat_facets;
        bool m_cornell_box;

        Ui::PainterParametersFor3d ui;

        void done(int r) override;
};
}

namespace dialog
{
[[nodiscard]] bool painter_parameters_for_3d(QWidget* parent, int max_thread_count, int width, int height, int max_screen_size,
                                             int default_samples_per_pixel, int max_samples_per_pixel, int* thread_count,
                                             int* paint_width, int* paint_height, int* samples_per_pixel, bool* flat_facets,
                                             bool* cornell_box);
}
