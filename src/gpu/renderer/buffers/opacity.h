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

#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
class Opacity
{
protected:
        ~Opacity() = default;

public:
        [[nodiscard]] virtual const std::vector<vulkan::ImageWithMemory>& images() const = 0;

        [[nodiscard]] virtual std::vector<VkClearValue> clear_values() const = 0;
};

class OpacityBuffers final : public Opacity
{
        unsigned image_count_;
        std::vector<vulkan::ImageWithMemory> images_;

        [[nodiscard]] const std::vector<vulkan::ImageWithMemory>& images() const override;
        [[nodiscard]] std::vector<VkClearValue> clear_values() const override;

public:
        explicit OpacityBuffers(bool ray_tracing);

        void create_buffers(
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& family_indices,
                VkSampleCountFlagBits sample_count,
                unsigned width,
                unsigned height);

        void delete_buffers();
};
}
