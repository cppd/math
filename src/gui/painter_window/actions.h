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

#include "pixels.h"

#include "../com/connection.h"
#include "../com/threads.h"

#include <QMenu>
#include <QStatusBar>
#include <memory>
#include <string>
#include <vector>

namespace ns::gui::painter_window
{
class Actions final
{
        const Pixels* m_pixels;
        std::unique_ptr<WorkerThreads> m_worker_threads;
        std::vector<Connection> m_connections;

        void save_to_file(const std::string& action) const;
        void save_all_to_files(const std::string& action) const;
        void add_volume(const std::string& action) const;

public:
        Actions(const Pixels* pixels, QMenu* menu, QStatusBar* status_bar);

        ~Actions();

        void set_progresses();
};
}
