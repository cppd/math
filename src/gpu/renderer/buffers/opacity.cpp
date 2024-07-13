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

#include "opacity.h"

#include <src/com/error.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
// clang-format off
constexpr std::array FORMATS
{
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT
};
constexpr std::array CLEAR_VALUES
{
        VkClearValue{.color{.uint32{0, 0, 0, 0}}},
        VkClearValue{.color{.float32{0, 0, 0, 0}}},
        VkClearValue{.color{.float32{0, 0, 0, 0}}},
        VkClearValue{.color{.float32{0, 0, 0, 0}}}
};
// clang-format on

constexpr VkImageUsageFlags USAGE_FLAGS = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
constexpr VkImageType IMAGE_TYPE = VK_IMAGE_TYPE_2D;
}

OpacityBuffers::OpacityBuffers(const bool ray_tracing)
        : image_count_(ray_tracing ? 4 : 2)
{
}

void OpacityBuffers::create_buffers(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const VkSampleCountFlagBits sample_count,
        const unsigned width,
        const unsigned height)
{
        delete_buffers();

        const VkExtent3D extent = vulkan::make_extent(width, height);

        images_.reserve(image_count_);
        for (std::size_t i = 0; i < image_count_; ++i)
        {
                images_.emplace_back(
                        device, family_indices, std::vector({FORMATS[i]}), sample_count, IMAGE_TYPE, extent,
                        USAGE_FLAGS);
        }
}

void OpacityBuffers::delete_buffers()
{
        images_.clear();
}

const std::vector<vulkan::ImageWithMemory>& OpacityBuffers::images() const
{
        ASSERT(images_.size() == image_count_);
        return images_;
}

std::vector<VkClearValue> OpacityBuffers::clear_values() const
{
        ASSERT(images_.size() == image_count_);
        std::vector<VkClearValue> res;
        res.reserve(image_count_);
        for (std::size_t i = 0; i < image_count_; ++i)
        {
                res.push_back(CLEAR_VALUES[i]);
        }
        return res;
}
}
