/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/view/event.h>
#include <src/view/view.h>

#include <QColor>
#include <QObject>
#include <QWidget>

#include <vector>

namespace ns::gui::main_window
{
class ColorsWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ColorsWidget ui_;

        view::View* view_ = nullptr;

        QColor background_color_;
        QColor wireframe_color_;
        QColor clip_plane_color_;
        QColor normal_color_positive_;
        QColor normal_color_negative_;
        QColor dft_background_color_;
        QColor dft_color_;

        void on_background_color_clicked();
        void on_wireframe_color_clicked();
        void on_clip_plane_color_clicked();
        void on_normal_color_positive_clicked();
        void on_normal_color_negative_clicked();
        void on_dft_background_color_clicked();
        void on_dft_color_clicked();

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

        [[nodiscard]] std::vector<view::Command> commands() const;

        [[nodiscard]] color::Color background_color() const;
};
}
