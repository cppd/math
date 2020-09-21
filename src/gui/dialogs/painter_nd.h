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

#include "ui_painter_nd.h"

#include <string>

namespace gui::dialog
{
namespace painter_parameters_for_nd_implementation
{
class PainterParametersForNd final : public QDialog
{
        Q_OBJECT

private:
        Ui::PainterParametersForNd ui;

        int m_max_thread_count;
        int m_min_screen_size;
        int m_max_screen_size;
        int m_max_samples_per_pixel;

        int m_thread_count;
        int m_min_size;
        int m_max_size;
        int m_samples_per_pixel;
        bool m_flat_facets;

        void on_min_size_changed(int);
        void on_max_size_changed(int);

        void done(int r) override;

public:
        explicit PainterParametersForNd(QWidget* parent = nullptr);

        [[nodiscard]] bool show(
                int dimension,
                int max_thread_count,
                int default_screen_size,
                int min_screen_size,
                int max_screen_size,
                int default_samples_per_pixel,
                int max_samples_per_pixel,
                int* thread_count,
                int* min_size,
                int* max_size,
                int* samples_per_pixel,
                bool* flat_facets);
};
}

[[nodiscard]] bool painter_parameters_for_nd(
        int dimension,
        int max_thread_count,
        int default_screen_size,
        int min_screen_size,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        int* thread_count,
        int* min_size,
        int* max_size,
        int* samples_per_pixel,
        bool* flat_facets);
}
