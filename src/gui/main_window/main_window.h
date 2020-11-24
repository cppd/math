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

#include "actions.h"
#include "colors_widget.h"
#include "graphics_widget.h"
#include "log.h"
#include "mesh_widget.h"
#include "model_tree.h"
#include "view_widget.h"
#include "volume_widget.h"

#include "ui_main_window.h"

#include <src/application/model_events.h>
#include <src/progress/progress_list.h>
#include <src/storage/repository.h>
#include <src/view/interface.h>

#include <QTimer>
#include <list>
#include <memory>
#include <string>
#include <thread>

namespace gui::main_window
{
class MainWindow final : public QMainWindow
{
        Q_OBJECT

private:
        const std::thread::id m_thread_id = std::this_thread::get_id();
        bool m_first_show = true;

        Ui::MainWindow ui;

        std::unique_ptr<Log> m_log;

        GraphicsWidget* m_graphics_widget;
        std::unique_ptr<view::View> m_view;

        std::unique_ptr<storage::Repository> m_repository;

        std::unique_ptr<ModelTree> m_model_tree;

        std::unique_ptr<ColorsWidget> m_colors_widget;
        std::unique_ptr<ViewWidget> m_view_widget;
        std::unique_ptr<MeshWidget> m_mesh_widget;
        std::unique_ptr<VolumeWidget> m_volume_widget;

        std::unique_ptr<application::ModelEvents> m_model_events;
        std::unique_ptr<Actions> m_actions;

        QTimer m_timer;

        void on_about_triggered();
        void on_exit_triggered();
        void on_graphics_widget_mouse_move(QMouseEvent*);
        void on_graphics_widget_mouse_press(QMouseEvent*);
        void on_graphics_widget_mouse_release(QMouseEvent*);
        void on_graphics_widget_mouse_wheel(QWheelEvent*);
        void on_graphics_widget_resize(QResizeEvent*);
        void on_help_triggered();
        void on_timer();

        void constructor_graphics_widget();
        void constructor_objects();
        void first_shown();

        std::vector<view::Command> view_initial_commands() const;

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void terminate_all_threads();

public:
        MainWindow();
        ~MainWindow() override;
};
}
