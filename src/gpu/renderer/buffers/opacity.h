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

#include <src/vulkan/buffers.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer
{
class Opacity
{
public:
        virtual ~Opacity() = default;

        virtual const std::vector<vulkan::ImageWithMemory>& images() const = 0;

        virtual std::vector<VkClearValue> clear_values() const = 0;
};

class OpacityBuffers final : public Opacity
{
        std::vector<vulkan::ImageWithMemory> images_;

        const std::vector<vulkan::ImageWithMemory>& images() const override;
        std::vector<VkClearValue> clear_values() const override;

public:
        void create_buffers(
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& family_indices,
                VkSampleCountFlagBits sample_count,
                unsigned width,
                unsigned height);

        void delete_buffers();
};
}
