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

#include "ui_colors_widget.h"

#include <src/color/color.h>
#include <src/view/interface.h>

#include <tuple>

namespace ns::gui::main_window
{
class ColorsWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ColorsWidget ui;

        view::View* m_view = nullptr;

        color::Spectrum m_lighting_spectrum;
        color::Color m_lighting_rgb;

        QColor m_background_color;
        QColor m_wireframe_color;
        QColor m_clip_plane_color;
        QColor m_normal_color_positive;
        QColor m_normal_color_negative;
        QColor m_dft_background_color;
        QColor m_dft_color;

        double lighting_intensity() const;
        color::Spectrum lighting_spectrum() const;
        color::Color lighting_rgb() const;

        void on_lighting_intensity_changed(int);
        void on_background_color_clicked();
        void on_wireframe_color_clicked();
        void on_clip_plane_color_clicked();
        void on_normal_color_positive_clicked();
        void on_normal_color_negative_clicked();
        void on_dft_background_color_clicked();
        void on_dft_color_clicked();

        void set_lighting_color(double intensity, bool set_slider);
        void set_background_color(const QColor& c);
        void set_wireframe_color(const QColor& c);
        void set_clip_plane_color(const QColor& c);
        void set_normal_color_positive(const QColor& c);
        void set_normal_color_negative(const QColor& c);
        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

public:
        ColorsWidget();

        void set_view(view::View* view);

        std::tuple<color::Spectrum, color::Color> lighting_color() const;
        color::Color background_color() const;
        color::Color wireframe_color() const;
        color::Color clip_plane_color() const;
        color::Color normal_color_positive() const;
        color::Color normal_color_negative() const;
        color::Color dft_background_color() const;
        color::Color dft_color() const;
};
}
