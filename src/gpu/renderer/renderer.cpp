/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "renderer.h"

#include "functionality.h"
#include "renderer_impl.h"

#include "code/code.h"
#include "test/ray_tracing/test_ray_tracing.h"

#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>

#include <memory>

namespace ns::gpu::renderer
{
namespace
{
constexpr bool RAY_TRACING = true;
}

vulkan::physical_device::DeviceFunctionality Renderer::device_functionality()
{
        vulkan::physical_device::DeviceFunctionality res = renderer::device_functionality();
        if (RAY_TRACING)
        {
                res.merge(renderer::device_ray_tracing_functionality());
        }
        return res;
}

std::unique_ptr<Renderer> create_renderer(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue,
        const bool sample_shading,
        const bool sampler_anisotropy)
{
        const bool ray_tracing = ray_tracing_supported(*device);

        if (ray_tracing)
        {
                test::test_ray_tracing(*device, *compute_queue);
        }

        const Code code(ray_tracing);

        return std::make_unique<RendererImpl>(
                device, code, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                compute_command_pool, compute_queue, sample_shading, sampler_anisotropy);
}
}
