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

#include "objects.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
enum class PhysicalDeviceFeatures
{
        alphaToOne,
        depthBiasClamp,
        depthBounds,
        depthClamp,
        drawIndirectFirstInstance,
        dualSrcBlend,
        fillModeNonSolid,
        fragmentStoresAndAtomics,
        fullDrawIndexUint32,
        geometryShader,
        imageCubeArray,
        independentBlend,
        inheritedQueries,
        largePoints,
        logicOp,
        multiDrawIndirect,
        multiViewport,
        occlusionQueryPrecise,
        pipelineStatisticsQuery,
        robustBufferAccess,
        sampleRateShading,
        samplerAnisotropy,
        shaderClipDistance,
        shaderCullDistance,
        shaderFloat64,
        shaderImageGatherExtended,
        shaderInt16,
        shaderInt64,
        shaderResourceMinLod,
        shaderResourceResidency,
        shaderSampledImageArrayDynamicIndexing,
        shaderStorageBufferArrayDynamicIndexing,
        shaderStorageImageArrayDynamicIndexing,
        shaderStorageImageExtendedFormats,
        shaderStorageImageMultisample,
        shaderStorageImageReadWithoutFormat,
        shaderStorageImageWriteWithoutFormat,
        shaderTessellationAndGeometryPointSize,
        shaderUniformBufferArrayDynamicIndexing,
        sparseBinding,
        sparseResidency16Samples,
        sparseResidency2Samples,
        sparseResidency4Samples,
        sparseResidency8Samples,
        sparseResidencyAliased,
        sparseResidencyBuffer,
        sparseResidencyImage2D,
        sparseResidencyImage3D,
        tessellationShader,
        textureCompressionASTC_LDR,
        textureCompressionBC,
        textureCompressionETC2,
        variableMultisampleRate,
        vertexPipelineStoresAndAtomics,
        wideLines,
        //
        multiview,
        multiviewGeometryShader,
        multiviewTessellationShader,
        protectedMemory,
        samplerYcbcrConversion,
        shaderDrawParameters,
        storageBuffer16BitAccess,
        storageInputOutput16,
        storagePushConstant16,
        uniformAndStorageBuffer16BitAccess,
        variablePointers,
        variablePointersStorageBuffer,
        //
        bufferDeviceAddress,
        bufferDeviceAddressCaptureReplay,
        bufferDeviceAddressMultiDevice,
        descriptorBindingPartiallyBound,
        descriptorBindingSampledImageUpdateAfterBind,
        descriptorBindingStorageBufferUpdateAfterBind,
        descriptorBindingStorageImageUpdateAfterBind,
        descriptorBindingStorageTexelBufferUpdateAfterBind,
        descriptorBindingUniformBufferUpdateAfterBind,
        descriptorBindingUniformTexelBufferUpdateAfterBind,
        descriptorBindingUpdateUnusedWhilePending,
        descriptorBindingVariableDescriptorCount,
        descriptorIndexing,
        drawIndirectCount,
        hostQueryReset,
        imagelessFramebuffer,
        runtimeDescriptorArray,
        samplerFilterMinmax,
        samplerMirrorClampToEdge,
        scalarBlockLayout,
        separateDepthStencilLayouts,
        shaderBufferInt64Atomics,
        shaderFloat16,
        shaderInputAttachmentArrayDynamicIndexing,
        shaderInputAttachmentArrayNonUniformIndexing,
        shaderInt8,
        shaderOutputLayer,
        shaderOutputViewportIndex,
        shaderSampledImageArrayNonUniformIndexing,
        shaderSharedInt64Atomics,
        shaderStorageBufferArrayNonUniformIndexing,
        shaderStorageImageArrayNonUniformIndexing,
        shaderStorageTexelBufferArrayDynamicIndexing,
        shaderStorageTexelBufferArrayNonUniformIndexing,
        shaderSubgroupExtendedTypes,
        shaderUniformBufferArrayNonUniformIndexing,
        shaderUniformTexelBufferArrayDynamicIndexing,
        shaderUniformTexelBufferArrayNonUniformIndexing,
        storageBuffer8BitAccess,
        storagePushConstant8,
        subgroupBroadcastDynamicId,
        timelineSemaphore,
        uniformAndStorageBuffer8BitAccess,
        uniformBufferStandardLayout,
        vulkanMemoryModel,
        vulkanMemoryModelAvailabilityVisibilityChains,
        vulkanMemoryModelDeviceScope
};

class PhysicalDevice final
{
        VkPhysicalDevice m_physical_device;
        DeviceFeatures m_features;
        DeviceProperties m_properties;
        std::vector<VkQueueFamilyProperties> m_queue_families;
        std::vector<bool> m_presentation_supported;
        std::unordered_set<std::string> m_supported_extensions;

public:
        PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

        operator VkPhysicalDevice() const&;
        operator VkPhysicalDevice() const&& = delete;

        const DeviceFeatures& features() const;
        const DeviceProperties& properties() const;
        const std::vector<VkQueueFamilyProperties>& queue_families() const;
        const std::unordered_set<std::string>& supported_extensions() const;

        uint32_t family_index(VkQueueFlags set_flags, VkQueueFlags not_set_flags, VkQueueFlags default_flags) const;
        uint32_t presentation_family_index() const;
        bool supports_extensions(const std::vector<std::string>& extensions) const;
        bool queue_family_supports_presentation(uint32_t index) const;
};

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance);

PhysicalDevice create_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        int api_version_major,
        int api_version_minor,
        const std::vector<std::string>& required_extensions,
        const std::vector<PhysicalDeviceFeatures>& required_features);

Device create_device(
        const PhysicalDevice& physical_device,
        const std::unordered_map<uint32_t, uint32_t>& queue_families,
        const std::vector<std::string>& required_extensions,
        const std::vector<PhysicalDeviceFeatures>& required_features,
        const std::vector<PhysicalDeviceFeatures>& optional_features);
}
