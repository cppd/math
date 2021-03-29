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

#include <src/color/color.h>
#include <src/image/format.h>
#include <src/progress/progress_list.h>

#include <functional>
#include <vector>

namespace ns::gui::painter_window
{
std::function<void(ProgressRatioList*)> save_to_file(
        const std::vector<int>& size,
        const Color& background,
        image::ColorFormat color_format,
        std::vector<std::byte>&& pixels);

std::function<void(ProgressRatioList*)> save_all_to_files(
        const std::vector<int>& size,
        const Color& background,
        image::ColorFormat color_format,
        std::vector<std::byte>&& pixels);

std::function<void(ProgressRatioList*)> add_volume(
        const std::vector<int>& size,
        const Color& background,
        image::ColorFormat color_format,
        std::vector<std::byte>&& pixels);
}
