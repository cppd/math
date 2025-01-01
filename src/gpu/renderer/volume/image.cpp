/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "image.h"

#include <src/color/rgb8.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/image/conversion.h>
#include <src/image/format.h>
#include <src/image/image.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstring>
#include <functional>
#include <vector>

namespace ns::gpu::renderer
{
std::vector<VkFormat> volume_transfer_function_formats(const image::ColorFormat color_format)
{
        switch (color_format)
        {
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R16G16B16A16_SRGB:
        case image::ColorFormat::R32G32B32A32:
                return {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
        case image::ColorFormat::R8_SRGB:
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R16G16B16_SRGB:
        case image::ColorFormat::R32G32B32:
        case image::ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case image::ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case image::ColorFormat::R32G32B32A32_PREMULTIPLIED:
                error("Unsupported transfer function format: " + image::format_to_string(color_format));
        }

        error_fatal("Unknown color format " + image::format_to_string(color_format));
}

std::vector<VkFormat> volume_image_formats(const image::ColorFormat color_format)
{
        switch (color_format)
        {
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
                return {VK_FORMAT_R16_UNORM, VK_FORMAT_R32_SFLOAT};
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R16G16B16_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R16G16B16A16_SRGB:
        case image::ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case image::ColorFormat::R32G32B32:
        case image::ColorFormat::R32G32B32A32:
        case image::ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return {VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R8_SRGB:
                error("Unsupported volume image format: " + image::format_to_string(color_format));
        }

        error_fatal("Unknown color format " + image::format_to_string(color_format));
}

void write_volume_image(
        const image::Image<3>& image,
        const std::function<void(image::ColorFormat color_format, const std::vector<std::byte>& pixels)>& write)
{
        switch (image.color_format)
        {
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R16G16B16A16_SRGB:
        case image::ColorFormat::R32G32B32A32:
                write(image.color_format, image.pixels);
                return;
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R16G16B16_SRGB:
        case image::ColorFormat::R32G32B32:
        case image::ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case image::ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case image::ColorFormat::R32G32B32A32_PREMULTIPLIED:
        {
                constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R32G32B32A32;
                std::vector<std::byte> pixels;
                image::format_conversion(image.color_format, image.pixels, COLOR_FORMAT, &pixels);
                write(COLOR_FORMAT, pixels);
                return;
        }
        case image::ColorFormat::R8_SRGB:
                error("Unsupported volume image format: " + image::format_to_string(image.color_format));
        }

        error_fatal("Unknown color format " + image::format_to_string(image.color_format));
}

bool is_scalar_volume(const image::ColorFormat color_format)
{
        return 1 == image::format_component_count(color_format);
}

image::Image<1> volume_transfer_function()
{
        constexpr int SIZE = 256;

        constexpr color::RGB8 COLOR{230, 255, 230};
        constexpr float RED = COLOR.linear_red();
        constexpr float GREEN = COLOR.linear_green();
        constexpr float BLUE = COLOR.linear_blue();

        std::vector<float> pixels;
        pixels.reserve(4ull * SIZE);
        constexpr float MAX = SIZE - 1;
        for (int i = 0; i < SIZE; ++i)
        {
                const float alpha = i / MAX;
                pixels.push_back(RED);
                pixels.push_back(GREEN);
                pixels.push_back(BLUE);
                pixels.push_back(alpha);
        }

        image::Image<1> image;

        image.pixels.resize(data_size(pixels));
        std::memcpy(image.pixels.data(), data_pointer(pixels), data_size(pixels));
        image.color_format = image::ColorFormat::R32G32B32A32;
        image.size[0] = SIZE;

        return image;
}
}
