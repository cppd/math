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

#include "pixels.h"

#include "../com/connection.h"
#include "../com/threads.h"

#include <QMenu>
#include <QStatusBar>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ns::gui::painter_window
{
class Actions final
{
        const Pixels* pixels_;
        std::unique_ptr<WorkerThreads> worker_threads_;
        std::vector<Connection> connections_;

        void save_image(const std::string& action, long long slice) const;
        void save_image(const std::string& action) const;
        void add_volume(const std::string& action) const;

public:
        Actions(const Pixels* pixels, QMenu* menu, QStatusBar* status_bar, std::function<long long()> slice_number);

        ~Actions();

        void set_progresses();
};
}
