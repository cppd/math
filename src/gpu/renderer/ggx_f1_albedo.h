/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer
{
class GgxF1Albedo final
{
        vulkan::ImageWithMemory cosine_roughness_;
        vulkan::ImageWithMemory cosine_weighted_average_;
        vulkan::handle::Sampler sampler_;

public:
        GgxF1Albedo(
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& graphics_family_indices,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue);

        const vulkan::ImageWithMemory& cosine_roughness() const;
        const vulkan::ImageWithMemory& cosine_weighted_average() const;
        const vulkan::handle::Sampler& sampler() const;
};
}
