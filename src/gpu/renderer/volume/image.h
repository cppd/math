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

#include <src/image/format.h>
#include <src/image/image.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <functional>
#include <vector>

namespace ns::gpu::renderer
{
std::vector<VkFormat> volume_transfer_function_formats(image::ColorFormat color_format);

std::vector<VkFormat> volume_image_formats(image::ColorFormat color_format);

void write_volume_image(
        const image::Image<3>& image,
        const std::function<void(image::ColorFormat color_format, const std::vector<std::byte>& pixels)>& write);

bool is_scalar_volume(image::ColorFormat color_format);

image::Image<1> volume_transfer_function();
}
