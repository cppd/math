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

#include "../com/threads.h"

#include <QStatusBar>
#include <memory>
#include <vector>

namespace gui::painter_window
{
class Actions final
{
        std::vector<int> m_screen_size;

        const std::vector<std::byte>* m_pixels_bgra;
        size_t m_slice_size;
        const long long* m_slice_offset;

        std::unique_ptr<WorkerThreads> m_worker_threads;

public:
        Actions(std::vector<int> screen_size,
                const std::vector<std::byte>* pixels_bgra,
                size_t slice_size,
                const long long* slice_offset,
                QStatusBar* status_bar);

        ~Actions();

        void set_progresses();

        void save_to_file(bool without_background) const;
        void save_all_to_files(bool without_background) const;
        void add_volume(bool without_background) const;
};
}
