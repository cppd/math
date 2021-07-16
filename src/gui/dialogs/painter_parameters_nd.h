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

#include "painter_parameters.h"

#include "ui_painter_parameters_nd.h"

#include <array>
#include <optional>
#include <tuple>

namespace ns::gui::dialog
{
struct PainterParametersNd final
{
        int min_size;
        int max_size;
};

class PainterParametersNdDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::PainterParametersNdDialog ui;

        PainterParametersWidget* m_parameters_widget;

        int m_min_screen_size;
        int m_max_screen_size;

        std::optional<std::tuple<PainterParameters, PainterParametersNd>>& m_parameters;

        PainterParametersNdDialog(
                int dimension,
                int max_thread_count,
                int default_screen_size,
                int min_screen_size,
                int max_screen_size,
                int default_samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int default_precision_index,
                const std::array<const char*, 2>& colors,
                int default_color_index,
                std::optional<std::tuple<PainterParameters, PainterParametersNd>>& parameters);

        void on_min_size_changed(int);
        void on_max_size_changed(int);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<std::tuple<PainterParameters, PainterParametersNd>> show(
                int dimension,
                int max_thread_count,
                int default_screen_size,
                int min_screen_size,
                int max_screen_size,
                int default_samples_per_pixel,
                int max_samples_per_pixel,
                const std::array<const char*, 2>& precisions,
                int default_precision_index,
                const std::array<const char*, 2>& colors,
                int default_color_index);
};
}
