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

#include <src/progress/progress_list.h>

#include <functional>
#include <vector>

namespace gui::painter_window
{
std::function<void(ProgressRatioList*)> save_to_file(
        const std::vector<int>& screen_size,
        bool without_background,
        std::vector<std::byte>&& pixels_rgba);

std::function<void(ProgressRatioList*)> save_all_to_files(
        const std::vector<int>& screen_size,
        bool without_background,
        std::vector<std::byte>&& pixels_rgba);

std::function<void(ProgressRatioList*)> add_volume(
        const std::vector<int>& screen_size,
        std::vector<std::byte>&& pixels_rgba);
}
