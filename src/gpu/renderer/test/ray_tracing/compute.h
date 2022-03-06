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

#include "image.h"

#include <src/image/image.h>

namespace ns::gpu::renderer::test
{
image::Image<2> ray_tracing(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const RayTracingImage& ray_tracing_image,
        VkAccelerationStructureKHR acceleration_structure,
        const std::string_view& file_name);

image::Image<2> ray_query(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const RayTracingImage& ray_tracing_image,
        VkAccelerationStructureKHR acceleration_structure,
        const std::string_view& file_name);
}