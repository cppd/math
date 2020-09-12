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

#include "threads.h"

#include "../com/connection.h"

#include <src/storage/repository.h>

#include <QMenu>
#include <string>
#include <vector>

namespace gui
{
class Actions final
{
        std::vector<Connection> m_connections;
        WorkerThreads* m_threads;

public:
        Actions(QMenu* menu, const storage::Repository* repository, WorkerThreads* threads);

        void connect_load_action(QAction* action);
        void load_from_file(const std::string& file_name, bool use_object_selection_dialog);
};
}
