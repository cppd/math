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

#include <src/text/glyphs.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::text_writer
{
class Glyphs final
{
        std::unordered_map<char32_t, text::FontGlyph> glyphs_;
        vulkan::ImageWithMemory image_;
        unsigned size_;

        Glyphs(unsigned size,
               const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const std::vector<std::uint32_t>& family_indices,
               text::FontGlyphs&& font_glyphs);

public:
        Glyphs(unsigned size,
               const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] unsigned size() const
        {
                return size_;
        }

        [[nodiscard]] const std::unordered_map<char32_t, text::FontGlyph>& glyphs() const
        {
                return glyphs_;
        }

        [[nodiscard]] VkImageView image_view() const
        {
                return image_.image_view().handle();
        }
};
}
