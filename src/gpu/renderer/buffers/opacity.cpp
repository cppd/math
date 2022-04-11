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

#include "opacity.h"

#include <src/com/error.h>

namespace ns::gpu::renderer
{
void OpacityBuffers::create_buffers(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const VkSampleCountFlagBits sample_count,
        const unsigned width,
        const unsigned height)
{
        delete_buffers();

        constexpr VkImageUsageFlags USAGE_FLAGS = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        constexpr VkImageType IMAGE_TYPE = VK_IMAGE_TYPE_2D;

        const VkExtent3D extent = vulkan::make_extent(width, height);
        const std::vector<VkFormat> formats = {VK_FORMAT_R32G32B32A32_SFLOAT};

        images_.reserve(2);
        images_.emplace_back(device, family_indices, formats, sample_count, IMAGE_TYPE, extent, USAGE_FLAGS);
        images_.emplace_back(device, family_indices, formats, sample_count, IMAGE_TYPE, extent, USAGE_FLAGS);
}

void OpacityBuffers::delete_buffers()
{
        images_.clear();
}

const std::vector<vulkan::ImageWithMemory>& OpacityBuffers::images() const
{
        ASSERT(images_.size() == 2);
        return images_;
}
}