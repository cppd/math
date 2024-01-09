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

#include <src/gui/com/connection.h>
#include <src/gui/com/threads.h>
#include <src/storage/repository.h>

#include <QMenu>

#include <vector>

namespace ns::gui::main_window
{
void create_repository_menu(
        unsigned thread_id,
        std::vector<Connection>* connections,
        WorkerThreads* threads,
        QMenu* menu_create,
        const storage::Repository* repository);
}
