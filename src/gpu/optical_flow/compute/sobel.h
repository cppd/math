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

#pragma once

#include <src/gpu/optical_flow/shaders/sobel.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>

namespace ns::gpu::optical_flow::compute
{
class Sobel final
{
        VkDevice device_;

        SobelProgram sobel_program_;
        std::vector<SobelMemory> sobel_memory_;
        std::vector<numerical::Vector2i> sobel_groups_;

public:
        explicit Sobel(VkDevice device);

        void create_buffers(
                const std::vector<numerical::Vector2i>& sizes,
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images);

        void delete_buffers();

        void commands(
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy,
                int index,
                VkCommandBuffer command_buffer) const;
};
}
