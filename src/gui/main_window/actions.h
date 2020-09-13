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

#include "colors_widget.h"
#include "model_tree.h"
#include "threads.h"

#include "../com/command_line.h"
#include "../com/connection.h"

#include <src/storage/repository.h>
#include <src/view/interface.h>

#include <QAction>
#include <QMenu>
#include <string>
#include <thread>
#include <vector>

namespace gui
{
class Actions final
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::vector<Connection> m_connections;

public:
        Actions(const CommandLineOptions& options,
                QAction* action_load,
                QAction* action_export,
                QAction* action_self_test,
                QMenu* menu_create,
                QMenu* menu_edit,
                QMenu* menu_rendering,
                const storage::Repository* repository,
                WorkerThreads* threads,
                view::View* view,
                ModelTree* model_tree,
                const ColorsWidget* colors);
};
}
