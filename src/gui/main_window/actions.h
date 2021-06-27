/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../com/command_line.h"
#include "../com/connection.h"
#include "../com/threads.h"

#include <src/storage/repository.h>
#include <src/view/interface.h>

#include <QAction>
#include <QMenu>
#include <QStatusBar>
#include <string>
#include <vector>

namespace ns::gui::main_window
{
class Actions final
{
        std::unique_ptr<WorkerThreads> m_worker_threads;
        std::vector<Connection> m_connections;

public:
        Actions(const CommandLineOptions& options,
                QStatusBar* status_bar,
                QAction* action_self_test,
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
