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

#include "ray_tracing.h"

#include "acceleration_structure.h"
#include "descriptors.h"
#include "pipeline.h"

namespace ns::gpu::renderer
{
void create_ray_tracing_data(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue)
{
        const AccelerationStructure bottom_level = create_bottom_level_acceleration_structure(
                *device, *compute_command_pool, *compute_queue, {compute_command_pool->family_index()});

        const AccelerationStructure top_level = create_top_level_acceleration_structure(
                *device, *compute_command_pool, *compute_queue, {compute_command_pool->family_index()}, bottom_level);

        const RayTracingPipeline ray_tracing_pipeline(*device);

        const RayTracingMemory memory(
                *device, ray_tracing_pipeline.descriptor_set_layout(),
                ray_tracing_pipeline.descriptor_set_layout_bindings());

        memory.set_acceleration_structure(top_level.handle());

        const vulkan::handle::Pipeline pipeline = ray_tracing_pipeline.create_pipeline();
}
}
