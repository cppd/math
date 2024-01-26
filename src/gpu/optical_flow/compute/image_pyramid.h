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

#include <src/gpu/optical_flow/shaders/downsample.h>
#include <src/gpu/optical_flow/shaders/grayscale.h>
#include <src/numerical/region.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <vector>

namespace ns::gpu::optical_flow::compute
{
class ImagePyramid final
{
        VkDevice device_;

        GrayscaleProgram grayscale_program_;
        GrayscaleMemory grayscale_memory_;
        Vector2i grayscale_groups_;

        DownsampleProgram downsample_program_;
        std::vector<DownsampleMemory> downsample_memory_;
        std::vector<Vector2i> downsample_groups_;

public:
        explicit ImagePyramid(VkDevice device);

        void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const numerical::Region<2, int>& rectangle,
                const std::vector<Vector2i>& sizes,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images);

        void delete_buffers();

        void commands(
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
                int index,
                VkCommandBuffer command_buffer) const;
};
}
