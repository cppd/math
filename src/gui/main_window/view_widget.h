/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "ui_view_widget.h"

#include <src/view/event.h>
#include <src/view/view.h>

#include <QObject>
#include <QWidget>

#include <vector>

namespace ns::gui::main_window
{
class ViewWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ViewWidget ui_;

        view::View* view_ = nullptr;

        void set_functionality(const view::info::Functionality& functionality);
        void set_sample_count(const view::info::SampleCount& sample_count);

        void on_clip_plane_clicked();
        void on_clip_plane_lines_clicked();
        void on_convex_hull_2d_clicked();
        void on_dft_clicked();
        void on_fog_clicked();
        void on_fps_clicked();
        void on_materials_clicked();
        void on_normals_clicked();
        void on_optical_flow_clicked();
        void on_pencil_sketch_clicked();
        void on_shadow_clicked();
        void on_flat_shading_clicked();
        void on_vertical_sync_clicked();
        void on_wireframe_clicked();
        void on_reset_view_clicked();
        void on_clip_plane_changed(int);
        void on_dft_brightness_changed(int);
        void on_normals_changed(int);
        void on_shadow_quality_changed(int);

        [[nodiscard]] double dft_brightness() const;
        [[nodiscard]] double shadow_zoom() const;
        [[nodiscard]] double normal_length() const;

public:
        ViewWidget();

        void set_view(view::View* view);

        [[nodiscard]] std::vector<view::Command> commands() const;
};
}
