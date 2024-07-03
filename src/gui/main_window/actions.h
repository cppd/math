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

#include "colors_widget.h"
#include "lighting_widget.h"
#include "model_tree.h"

#include <src/gui/com/command_line.h>
#include <src/gui/com/connection.h>
#include <src/gui/com/threads.h>
#include <src/storage/repository.h>
#include <src/view/view.h>

#include <QAction>
#include <QMenu>
#include <QStatusBar>

#include <memory>
#include <vector>

namespace ns::gui::main_window
{
class Actions final
{
        std::unique_ptr<com::WorkerThreads> worker_threads_;
        std::vector<com::Connection> connections_;

public:
        Actions(const com::CommandLineOptions& options,
                QStatusBar* status_bar,
                QAction* action_self_test,
                QAction* action_benchmark,
                QMenu* menu_file,
                QMenu* menu_create,
                QMenu* menu_edit,
                QMenu* menu_rendering,
                const storage::Repository* repository,
                view::View* view,
                ModelTree* model_tree,
                const LightingWidget* lighting,
                const ColorsWidget* colors);

        ~Actions();

        void set_progresses();
};
}
