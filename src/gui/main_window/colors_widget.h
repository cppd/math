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

#include "ui_colors_widget.h"

#include <src/color/color.h>
#include <src/view/interface.h>

namespace ns::gui::main_window
{
class ColorsWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ColorsWidget ui;

        view::View* m_view = nullptr;

        QColor m_background_color;
        QColor m_specular_color;
        QColor m_wireframe_color;
        QColor m_clip_plane_color;
        QColor m_normal_color_positive;
        QColor m_normal_color_negative;
        QColor m_dft_background_color;
        QColor m_dft_color;

        void on_reset_lighting_clicked();
        void on_lighting_intensity_changed(int);
        void on_background_color_clicked();
        void on_wireframe_color_clicked();
        void on_clip_plane_color_clicked();
        void on_normal_color_positive_clicked();
        void on_normal_color_negative_clicked();
        void on_dft_background_color_clicked();
        void on_dft_color_clicked();

        void set_background_color(const QColor& c);
        void set_specular_color(const QColor& c);
        void set_wireframe_color(const QColor& c);
        void set_clip_plane_color(const QColor& c);
        void set_normal_color_positive(const QColor& c);
        void set_normal_color_negative(const QColor& c);
        void set_dft_background_color(const QColor& c);
        void set_dft_color(const QColor& c);

public:
        ColorsWidget();

        void set_view(view::View* view);

        Color background_color() const;
        Color specular_color() const;
        Color wireframe_color() const;
        Color clip_plane_color() const;
        Color normal_color_positive() const;
        Color normal_color_negative() const;
        Color dft_background_color() const;
        Color dft_color() const;
        double lighting_intensity() const;
};
}
