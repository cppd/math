/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "glyphs.h"

#include <src/com/error.h>
#include <src/image/image.h>
#include <src/text/fonts.h>

namespace ns::gpu::text_writer
{
namespace
{
// clang-format off
constexpr std::array GRAYSCALE_IMAGE_FORMATS = std::to_array<VkFormat>
({
        VK_FORMAT_R8_SRGB,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R32_SFLOAT
});
// clang-format on

std::vector<unsigned char> font_data()
{
        const text::Fonts& fonts = text::Fonts::instance();

        std::vector<std::string> font_names = fonts.names();
        if (font_names.empty())
        {
                error("Fonts not found");
        }

        return fonts.data(font_names.front());
}
}

Glyphs::Glyphs(
        const unsigned size,
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const std::vector<std::uint32_t>& family_indices)
        : size_(size)
{
        ASSERT(std::find(family_indices.cbegin(), family_indices.cend(), graphics_queue.family_index())
               != family_indices.cend());

        text::Font font(size_, font_data());

        image::Image<2> image;

        const auto max_image_dimension = device.properties().properties_10.limits.maxImageDimension2D;

        create_font_glyphs(font, max_image_dimension, max_image_dimension, &glyphs_, &image);

        image_.emplace(
                device, family_indices,
                std::vector<VkFormat>(std::cbegin(GRAYSCALE_IMAGE_FORMATS), std::cend(GRAYSCALE_IMAGE_FORMATS)),
                VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, vulkan::make_extent(image.size[0], image.size[1]),
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        image_->write(
                graphics_command_pool, graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.color_format, image.pixels);
}
}