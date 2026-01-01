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

#include "model_tree.h"
#include "range_slider.h"

#include "ui_volume_widget.h"

#include <src/storage/types.h>

#include <QObject>
#include <QPointer>
#include <QWidget>

#include <memory>
#include <thread>
#include <vector>

namespace ns::gui::main_window
{
class VolumeWidget final : public QWidget
{
        Q_OBJECT

private:
        const std::thread::id thread_id_ = std::this_thread::get_id();

        Ui::VolumeWidget ui_;

        std::unique_ptr<RangeSlider> slider_levels_;
        std::vector<QPointer<QWidget>> widgets_;
        ModelTree* model_tree_ = nullptr;

        void on_isosurface_clicked();
        void on_isosurface_transparency_changed();
        void on_isovalue_changed();
        void on_color_clicked();
        void on_ambient_changed();
        void on_metalness_changed();
        void on_roughness_changed();
        void on_levels_changed(double, double);
        void on_transparency_changed();

        void on_model_tree_item_update();

        void ui_disable();
        void ui_set(const storage::VolumeObjectConst& object);

        void set_enabled(bool enabled) const;

public:
        VolumeWidget();

        void set_model_tree(ModelTree* model_tree);
};
}
