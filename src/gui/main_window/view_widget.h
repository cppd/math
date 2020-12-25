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

#include "ui_view_widget.h"

#include <src/view/interface.h>

namespace ns::gui::main_window
{
class ViewWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ViewWidget ui;

        view::View* m_view = nullptr;

        void on_clip_plane_clicked();
        void on_convex_hull_2d_clicked();
        void on_dft_clicked();
        void on_fog_clicked();
        void on_fps_clicked();
        void on_materials_clicked();
        void on_normals_clicked();
        void on_optical_flow_clicked();
        void on_pencil_sketch_clicked();
        void on_shadow_clicked();
        void on_smooth_clicked();
        void on_vertical_sync_clicked();
        void on_wireframe_clicked();
        void on_reset_view_clicked();
        void on_clip_plane_changed(int);
        void on_dft_brightness_changed(int);
        void on_normals_changed(int);
        void on_shadow_quality_changed(int);

public:
        ViewWidget();

        void set_view(view::View* view);

        double dft_brightness() const;
        double shadow_zoom() const;
        double normal_length() const;
        bool smooth_checked() const;
        bool wireframe_checked() const;
        bool shadow_checked() const;
        bool fog_checked() const;
        bool materials_checked() const;
        bool fps_checked() const;
        bool pencil_sketch_checked() const;
        bool dft_checked() const;
        bool convex_hull_2d_checked() const;
        bool optical_flow_checked() const;
        bool normals_checked() const;
        bool vertical_sync_checked() const;
};
}
