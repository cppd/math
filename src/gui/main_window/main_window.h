/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "actions.h"
#include "colors_widget.h"
#include "graphics_widget.h"
#include "lighting_widget.h"
#include "log.h"
#include "mesh_widget.h"
#include "model_tree.h"
#include "view_widget.h"
#include "volume_widget.h"

#include "ui_main_window.h"

#include <src/gui/com/model_events.h>
#include <src/storage/repository.h>
#include <src/view/event.h>
#include <src/view/view.h>
#include <src/vulkan/instance/instance.h>

#include <QCloseEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QObject>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSinglePointEvent>
#include <QTimer>
#include <QWheelEvent>

#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace ns::gui::main_window
{
class MainWindow final : public QMainWindow
{
        Q_OBJECT

private:
        const std::thread::id thread_id_ = std::this_thread::get_id();
        bool first_show_ = true;

        Ui::MainWindow ui_;

        std::unique_ptr<Log> log_;

        GraphicsWidget* graphics_widget_;
        std::unique_ptr<vulkan::Instance> vulkan_instance_;
        std::unique_ptr<view::View> view_;

        std::unique_ptr<storage::Repository> repository_;

        std::unique_ptr<ModelTree> model_tree_;

        std::unique_ptr<LightingWidget> lighting_widget_;
        std::unique_ptr<ColorsWidget> colors_widget_;
        std::unique_ptr<ViewWidget> view_widget_;
        std::unique_ptr<MeshWidget> mesh_widget_;
        std::unique_ptr<VolumeWidget> volume_widget_;

        std::unique_ptr<ModelEvents> model_events_;
        std::unique_ptr<Actions> actions_;

        QTimer timer_;

        [[nodiscard]] std::tuple<double, double> graphics_widget_position(const QSinglePointEvent* event) const;

        void on_graphics_widget_mouse_move(const QMouseEvent* event);
        void on_graphics_widget_mouse_press(const QMouseEvent* event);
        void on_graphics_widget_mouse_release(const QMouseEvent* event);
        void on_graphics_widget_mouse_wheel(const QWheelEvent* event);
        void on_graphics_widget_resize(const QResizeEvent* event);
        void on_timer();

        void constructor_graphics_widget();
        void constructor_objects();
        void first_shown();

        [[nodiscard]] std::vector<view::Command> view_initial_commands() const;

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void terminate_all_threads();

public:
        MainWindow();
        ~MainWindow() override;
};
}
