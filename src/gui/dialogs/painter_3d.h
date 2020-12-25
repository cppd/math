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

#include "ui_painter_3d.h"

#include <optional>

namespace ns::gui::dialog
{
struct Painter3dParameters final
{
        int thread_count;
        int width;
        int height;
        int samples_per_pixel;
        bool flat_facets;
        bool cornell_box;
};

class Painter3dParametersDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::Painter3dParametersDialog ui;

        int m_min_width;
        int m_max_width;
        int m_min_height;
        int m_max_height;
        double m_aspect_ratio;
        int m_max_thread_count;
        int m_max_samples_per_pixel;

        std::optional<Painter3dParameters>& m_parameters;

        Painter3dParametersDialog(
                int max_thread_count,
                int width,
                int height,
                int max_screen_size,
                int default_samples_per_pixel,
                int max_samples_per_pixel,
                std::optional<Painter3dParameters>& parameters);

        void on_width_value_changed(int);
        void on_height_value_changed(int);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<Painter3dParameters> show(
                int max_thread_count,
                int width,
                int height,
                int max_screen_size,
                int default_samples_per_pixel,
                int max_samples_per_pixel);
};
}
