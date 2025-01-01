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

#include "model_tree.h"

#include "ui_mesh_widget.h"

#include <src/storage/types.h>

#include <QObject>
#include <QPointer>
#include <QWidget>

#include <thread>
#include <vector>

namespace ns::gui::main_window
{
class MeshWidget final : public QWidget
{
        Q_OBJECT

private:
        const std::thread::id thread_id_ = std::this_thread::get_id();

        Ui::MeshWidget ui_;

        std::vector<QPointer<QWidget>> widgets_;
        ModelTree* model_tree_ = nullptr;

        void on_ambient_changed();
        void on_metalness_changed();
        void on_roughness_changed();
        void on_transparency_changed();
        void on_color_clicked();

        void on_model_tree_item_update();

        void ui_disable();
        void ui_set(const storage::MeshObjectConst& object);

        void set_enabled(bool enabled) const;

public:
        MeshWidget();

        void set_model_tree(ModelTree* model_tree);
};
}
