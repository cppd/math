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

#include "functionality.h"

#include <src/com/log.h>
#include <src/com/string/strings.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/physical_device/features.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/physical_device/info.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <string>

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

constexpr vulkan::physical_device::Features RAY_TRACING_FEATURES = []
{
        vulkan::physical_device::Features res;

        // res.features_12.descriptorIndexing = VK_TRUE;
        res.features_12.bufferDeviceAddress = VK_TRUE;
        res.features_13.maintenance4 = VK_TRUE;

        res.acceleration_structure.accelerationStructure = VK_TRUE;
        // res.acceleration_structure.descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE;

        res.ray_query.rayQuery = VK_TRUE;

        res.ray_tracing_pipeline.rayTracingPipeline = VK_TRUE;
        // res.ray_tracing_pipeline.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
        // res.ray_tracing_pipeline.rayTraversalPrimitiveCulling = VK_TRUE;

        return res;
}();
}

vulkan::physical_device::DeviceFunctionality device_ray_tracing_functionality()
{
        vulkan::physical_device::DeviceFunctionality res;

        res.optional_extensions.insert(RAY_TRACING_EXTENSIONS.cbegin(), RAY_TRACING_EXTENSIONS.cend());
        res.optional_features = RAY_TRACING_FEATURES;

        return res;
}

vulkan::physical_device::DeviceFunctionality device_functionality()
{
        vulkan::physical_device::DeviceFunctionality res;

        res.required_features.features_10.geometryShader = VK_TRUE;
        res.required_features.features_10.fragmentStoresAndAtomics = VK_TRUE;
        res.required_features.features_10.shaderStorageImageMultisample = VK_TRUE;
        res.required_features.features_10.shaderClipDistance = VK_TRUE;

        return res;
}

bool ray_tracing_supported(const vulkan::Device& device)
{
        for (const auto& extension : RAY_TRACING_EXTENSIONS)
        {
                if (!device.extensions().contains(extension))
                {
                        LOG(std::string("Renderer ray tracing extension is not supported ") + extension);
                        return false;
                }
        }

        if (!check_features(RAY_TRACING_FEATURES, device.features()))
        {
                LOG("Renderer ray tracing features are not supported: "
                    + strings_to_sorted_string(
                            vulkan::physical_device::features_to_strings(RAY_TRACING_FEATURES, true), ", "));
                return false;
        }

        LOG("Renderer ray tracing supported");
        return true;
}
}
