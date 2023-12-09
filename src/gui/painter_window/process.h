/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/image/format.h>
#include <src/progress/progress_list.h>

#include <cstddef>
#include <functional>
#include <vector>

namespace ns::gui::painter_window
{
std::function<void(progress::RatioList*)> save_image(
        int width,
        int height,
        image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba);

std::function<void(progress::RatioList*)> save_image(
        const std::vector<int>& size,
        image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba);

std::function<void(progress::RatioList*)> add_volume(
        const std::vector<int>& size,
        image::ColorFormat color_format_rgb,
        std::vector<std::byte>&& pixels_rgb,
        image::ColorFormat color_format_rgba,
        std::vector<std::byte>&& pixels_rgba);
}
