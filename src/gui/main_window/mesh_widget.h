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

#include "model_tree.h"

#include "ui_mesh_widget.h"

#include <QPointer>
#include <thread>
#include <vector>

namespace gui
{
class MeshWidget final : public QWidget
{
        Q_OBJECT

private:
        const std::thread::id m_thread_id = std::this_thread::get_id();
        const double m_maximum_specular_power;
        const double m_maximum_model_lighting;
        ModelTree* const m_model_tree;

        Ui::MeshWidget ui;

        std::vector<QPointer<QWidget>> m_widgets;

        void on_mesh_ambient_changed(int);
        void on_mesh_diffuse_changed(int);
        void on_mesh_specular_changed(int);
        void on_mesh_specular_power_changed(int);
        void on_mesh_transparency_changed(int);
        void on_mesh_color_clicked();

        void mesh_ui_disable();
        void mesh_ui_set(const storage::MeshObjectConst& object);

        void set_enabled(bool enabled) const;

public:
        MeshWidget(ModelTree* model_tree, double maximum_specular_power, double maximum_model_lighting);

        void update();
};
}
