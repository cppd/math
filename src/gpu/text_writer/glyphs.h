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

#pragma once

#include <src/text/glyphs.h>
#include <src/vulkan/buffers.h>

#include <optional>
#include <unordered_map>

namespace ns::gpu::text_writer
{
class Glyphs final
{
        std::unordered_map<char32_t, text::FontGlyph> glyphs_;
        std::optional<vulkan::ImageWithMemory> image_;
        unsigned size_;

public:
        Glyphs(unsigned size,
               const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const std::vector<std::uint32_t>& family_indices);

        unsigned size() const
        {
                return size_;
        }

        const std::unordered_map<char32_t, text::FontGlyph>& glyphs() const
        {
                return glyphs_;
        }

        VkImageView image_view() const
        {
                return image_->image_view();
        }
};
}
