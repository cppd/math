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

#include "functionality.h"

#include <src/vulkan/features.h>

#include <array>

namespace ns::gpu::renderer
{
namespace
{
// clang-format off
constexpr std::array RAY_TRACING_EXTENSIONS
{
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};
// clang-format on

constexpr vulkan::PhysicalDeviceFeatures RAY_TRACING_FEATURES = []
{
        vulkan::PhysicalDeviceFeatures res;

        // res.features_12.descriptorIndexing = VK_TRUE;
        res.features_12.bufferDeviceAddress = VK_TRUE;

        res.acceleration_structure.accelerationStructure = VK_TRUE;
        // res.acceleration_structure.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;

        res.ray_query.rayQuery = VK_TRUE;

        res.ray_tracing_pipeline.rayTracingPipeline = VK_TRUE;
        // res.ray_tracing_pipeline.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
        // res.ray_tracing_pipeline.rayTraversalPrimitiveCulling = VK_TRUE;

        return res;
}();

void add_ray_tracing_functionality(vulkan::DeviceFunctionality* const res)
{
        for (const auto& s : RAY_TRACING_EXTENSIONS)
        {
                res->optional_extensions.insert(s);
        }

        vulkan::add_features(&res->optional_features, RAY_TRACING_FEATURES);
}
}

vulkan::DeviceFunctionality device_functionality()
{
        vulkan::DeviceFunctionality res;

        res.required_features.features_10.geometryShader = VK_TRUE;
        res.required_features.features_10.fragmentStoresAndAtomics = VK_TRUE;
        res.required_features.features_10.shaderStorageImageMultisample = VK_TRUE;
        res.required_features.features_10.shaderClipDistance = VK_TRUE;

        add_ray_tracing_functionality(&res);

        return res;
}

bool ray_tracing_supported(const vulkan::Device& device)
{
        for (const auto& s : RAY_TRACING_EXTENSIONS)
        {
                if (!device.extensions().contains(s))
                {
                        return false;
                }
        }

        return check_features(RAY_TRACING_FEATURES, device.features());
}
}
