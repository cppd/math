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

#include "feature_properties.h"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <string>

namespace ns::vulkan
{
namespace
{
constexpr std::size_t SIZE = sizeof(VkBool32);

template <typename T>
constexpr std::size_t ceil_size(const std::size_t size)
{
        return ((size + alignof(T) - 1) / alignof(T)) * alignof(T);
}

template <typename T>
struct Check final
{
        static constexpr std::size_t COUNT = FeatureProperties<T>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<T>::OFFSET;
        static_assert(ceil_size<T>(COUNT * SIZE + OFFSET) == sizeof(T));
};

template struct Check<VkPhysicalDeviceFeatures>;
template struct Check<VkPhysicalDeviceVulkan11Features>;
template struct Check<VkPhysicalDeviceVulkan12Features>;
template struct Check<VkPhysicalDeviceVulkan13Features>;
template struct Check<VkPhysicalDeviceAccelerationStructureFeaturesKHR>;
template struct Check<VkPhysicalDeviceRayQueryFeaturesKHR>;
template struct Check<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>;
}

#define CASE_FEATURE(name, type, feature)                                        \
        case (offsetof(type, feature) - FeatureProperties<type>::OFFSET) / SIZE: \
                return name "::" #feature;

#define CASE_FEATURE_10(feature) CASE_FEATURE("Features", VkPhysicalDeviceFeatures, feature)

#define CASE_FEATURE_11(feature) CASE_FEATURE("Vulkan11Features", VkPhysicalDeviceVulkan11Features, feature)

#define CASE_FEATURE_12(feature) CASE_FEATURE("Vulkan12Features", VkPhysicalDeviceVulkan12Features, feature)

#define CASE_FEATURE_13(feature) CASE_FEATURE("Vulkan13Features", VkPhysicalDeviceVulkan13Features, feature)

#define CASE_ACCELERATION_STRUCTURE(feature) \
        CASE_FEATURE("AccelerationStructureFeaturesKHR", VkPhysicalDeviceAccelerationStructureFeaturesKHR, feature)

#define CASE_RAY_QUERY(feature) CASE_FEATURE("RayQueryFeaturesKHR", VkPhysicalDeviceRayQueryFeaturesKHR, feature)

#define CASE_RAY_TRACING_PIPELINE(feature) \
        CASE_FEATURE("RayTracingPipelineFeaturesKHR", VkPhysicalDeviceRayTracingPipelineFeaturesKHR, feature)

std::string FeatureProperties<VkPhysicalDeviceFeatures>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_FEATURE_10(robustBufferAccess)
                CASE_FEATURE_10(fullDrawIndexUint32)
                CASE_FEATURE_10(imageCubeArray)
                CASE_FEATURE_10(independentBlend)
                CASE_FEATURE_10(geometryShader)
                CASE_FEATURE_10(tessellationShader)
                CASE_FEATURE_10(sampleRateShading)
                CASE_FEATURE_10(dualSrcBlend)
                CASE_FEATURE_10(logicOp)
                CASE_FEATURE_10(multiDrawIndirect)
                CASE_FEATURE_10(drawIndirectFirstInstance)
                CASE_FEATURE_10(depthClamp)
                CASE_FEATURE_10(depthBiasClamp)
                CASE_FEATURE_10(fillModeNonSolid)
                CASE_FEATURE_10(depthBounds)
                CASE_FEATURE_10(wideLines)
                CASE_FEATURE_10(largePoints)
                CASE_FEATURE_10(alphaToOne)
                CASE_FEATURE_10(multiViewport)
                CASE_FEATURE_10(samplerAnisotropy)
                CASE_FEATURE_10(textureCompressionETC2)
                CASE_FEATURE_10(textureCompressionASTC_LDR)
                CASE_FEATURE_10(textureCompressionBC)
                CASE_FEATURE_10(occlusionQueryPrecise)
                CASE_FEATURE_10(pipelineStatisticsQuery)
                CASE_FEATURE_10(vertexPipelineStoresAndAtomics)
                CASE_FEATURE_10(fragmentStoresAndAtomics)
                CASE_FEATURE_10(shaderTessellationAndGeometryPointSize)
                CASE_FEATURE_10(shaderImageGatherExtended)
                CASE_FEATURE_10(shaderStorageImageExtendedFormats)
                CASE_FEATURE_10(shaderStorageImageMultisample)
                CASE_FEATURE_10(shaderStorageImageReadWithoutFormat)
                CASE_FEATURE_10(shaderStorageImageWriteWithoutFormat)
                CASE_FEATURE_10(shaderUniformBufferArrayDynamicIndexing)
                CASE_FEATURE_10(shaderSampledImageArrayDynamicIndexing)
                CASE_FEATURE_10(shaderStorageBufferArrayDynamicIndexing)
                CASE_FEATURE_10(shaderStorageImageArrayDynamicIndexing)
                CASE_FEATURE_10(shaderClipDistance)
                CASE_FEATURE_10(shaderCullDistance)
                CASE_FEATURE_10(shaderFloat64)
                CASE_FEATURE_10(shaderInt64)
                CASE_FEATURE_10(shaderInt16)
                CASE_FEATURE_10(shaderResourceResidency)
                CASE_FEATURE_10(shaderResourceMinLod)
                CASE_FEATURE_10(sparseBinding)
                CASE_FEATURE_10(sparseResidencyBuffer)
                CASE_FEATURE_10(sparseResidencyImage2D)
                CASE_FEATURE_10(sparseResidencyImage3D)
                CASE_FEATURE_10(sparseResidency2Samples)
                CASE_FEATURE_10(sparseResidency4Samples)
                CASE_FEATURE_10(sparseResidency8Samples)
                CASE_FEATURE_10(sparseResidency16Samples)
                CASE_FEATURE_10(sparseResidencyAliased)
                CASE_FEATURE_10(variableMultisampleRate)
                CASE_FEATURE_10(inheritedQueries)
        default:
                return "Unknown feature (index " + std::to_string(index) + ") in VkPhysicalDeviceFeatures)";
        }
}

std::string FeatureProperties<VkPhysicalDeviceVulkan11Features>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_FEATURE_11(storageBuffer16BitAccess)
                CASE_FEATURE_11(uniformAndStorageBuffer16BitAccess)
                CASE_FEATURE_11(storagePushConstant16)
                CASE_FEATURE_11(storageInputOutput16)
                CASE_FEATURE_11(multiview)
                CASE_FEATURE_11(multiviewGeometryShader)
                CASE_FEATURE_11(multiviewTessellationShader)
                CASE_FEATURE_11(variablePointersStorageBuffer)
                CASE_FEATURE_11(variablePointers)
                CASE_FEATURE_11(protectedMemory)
                CASE_FEATURE_11(samplerYcbcrConversion)
                CASE_FEATURE_11(shaderDrawParameters)
        default:
                return "Unknown feature (index " + std::to_string(index) + ") in VkPhysicalDeviceVulkan11Features)";
        }
}

std::string FeatureProperties<VkPhysicalDeviceVulkan12Features>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_FEATURE_12(samplerMirrorClampToEdge)
                CASE_FEATURE_12(drawIndirectCount)
                CASE_FEATURE_12(storageBuffer8BitAccess)
                CASE_FEATURE_12(uniformAndStorageBuffer8BitAccess)
                CASE_FEATURE_12(storagePushConstant8)
                CASE_FEATURE_12(shaderBufferInt64Atomics)
                CASE_FEATURE_12(shaderSharedInt64Atomics)
                CASE_FEATURE_12(shaderFloat16)
                CASE_FEATURE_12(shaderInt8)
                CASE_FEATURE_12(descriptorIndexing)
                CASE_FEATURE_12(shaderInputAttachmentArrayDynamicIndexing)
                CASE_FEATURE_12(shaderUniformTexelBufferArrayDynamicIndexing)
                CASE_FEATURE_12(shaderStorageTexelBufferArrayDynamicIndexing)
                CASE_FEATURE_12(shaderUniformBufferArrayNonUniformIndexing)
                CASE_FEATURE_12(shaderSampledImageArrayNonUniformIndexing)
                CASE_FEATURE_12(shaderStorageBufferArrayNonUniformIndexing)
                CASE_FEATURE_12(shaderStorageImageArrayNonUniformIndexing)
                CASE_FEATURE_12(shaderInputAttachmentArrayNonUniformIndexing)
                CASE_FEATURE_12(shaderUniformTexelBufferArrayNonUniformIndexing)
                CASE_FEATURE_12(shaderStorageTexelBufferArrayNonUniformIndexing)
                CASE_FEATURE_12(descriptorBindingUniformBufferUpdateAfterBind)
                CASE_FEATURE_12(descriptorBindingSampledImageUpdateAfterBind)
                CASE_FEATURE_12(descriptorBindingStorageImageUpdateAfterBind)
                CASE_FEATURE_12(descriptorBindingStorageBufferUpdateAfterBind)
                CASE_FEATURE_12(descriptorBindingUniformTexelBufferUpdateAfterBind)
                CASE_FEATURE_12(descriptorBindingStorageTexelBufferUpdateAfterBind)
                CASE_FEATURE_12(descriptorBindingUpdateUnusedWhilePending)
                CASE_FEATURE_12(descriptorBindingPartiallyBound)
                CASE_FEATURE_12(descriptorBindingVariableDescriptorCount)
                CASE_FEATURE_12(runtimeDescriptorArray)
                CASE_FEATURE_12(samplerFilterMinmax)
                CASE_FEATURE_12(scalarBlockLayout)
                CASE_FEATURE_12(imagelessFramebuffer)
                CASE_FEATURE_12(uniformBufferStandardLayout)
                CASE_FEATURE_12(shaderSubgroupExtendedTypes)
                CASE_FEATURE_12(separateDepthStencilLayouts)
                CASE_FEATURE_12(hostQueryReset)
                CASE_FEATURE_12(timelineSemaphore)
                CASE_FEATURE_12(bufferDeviceAddress)
                CASE_FEATURE_12(bufferDeviceAddressCaptureReplay)
                CASE_FEATURE_12(bufferDeviceAddressMultiDevice)
                CASE_FEATURE_12(vulkanMemoryModel)
                CASE_FEATURE_12(vulkanMemoryModelDeviceScope)
                CASE_FEATURE_12(vulkanMemoryModelAvailabilityVisibilityChains)
                CASE_FEATURE_12(shaderOutputViewportIndex)
                CASE_FEATURE_12(shaderOutputLayer)
                CASE_FEATURE_12(subgroupBroadcastDynamicId)
        default:
                return "Unknown feature (index " + std::to_string(index) + ") in VkPhysicalDeviceVulkan12Features)";
        }
}

std::string FeatureProperties<VkPhysicalDeviceVulkan13Features>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_FEATURE_13(robustImageAccess)
                CASE_FEATURE_13(inlineUniformBlock)
                CASE_FEATURE_13(descriptorBindingInlineUniformBlockUpdateAfterBind)
                CASE_FEATURE_13(pipelineCreationCacheControl)
                CASE_FEATURE_13(privateData)
                CASE_FEATURE_13(shaderDemoteToHelperInvocation)
                CASE_FEATURE_13(shaderTerminateInvocation)
                CASE_FEATURE_13(subgroupSizeControl)
                CASE_FEATURE_13(computeFullSubgroups)
                CASE_FEATURE_13(synchronization2)
                CASE_FEATURE_13(textureCompressionASTC_HDR)
                CASE_FEATURE_13(shaderZeroInitializeWorkgroupMemory)
                CASE_FEATURE_13(dynamicRendering)
                CASE_FEATURE_13(shaderIntegerDotProduct)
                CASE_FEATURE_13(maintenance4)
        default:
                return "Unknown feature (index " + std::to_string(index) + ") in VkPhysicalDeviceVulkan13Features)";
        }
}

std::string FeatureProperties<VkPhysicalDeviceAccelerationStructureFeaturesKHR>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_ACCELERATION_STRUCTURE(accelerationStructure)
                CASE_ACCELERATION_STRUCTURE(accelerationStructureCaptureReplay)
                CASE_ACCELERATION_STRUCTURE(accelerationStructureIndirectBuild)
                CASE_ACCELERATION_STRUCTURE(accelerationStructureHostCommands)
                CASE_ACCELERATION_STRUCTURE(descriptorBindingAccelerationStructureUpdateAfterBind)
        default:
                return "Unknown feature (index " + std::to_string(index)
                       + ") in VkPhysicalDeviceAccelerationStructureFeaturesKHR)";
        }
}

std::string FeatureProperties<VkPhysicalDeviceRayQueryFeaturesKHR>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_RAY_QUERY(rayQuery)
        default:
                return "Unknown feature (index " + std::to_string(index) + ") in VkPhysicalDeviceRayQueryFeaturesKHR)";
        }
}

std::string FeatureProperties<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>::name(const std::size_t index)
{
        switch (index)
        {
                CASE_RAY_TRACING_PIPELINE(rayTracingPipeline)
                CASE_RAY_TRACING_PIPELINE(rayTracingPipelineShaderGroupHandleCaptureReplay)
                CASE_RAY_TRACING_PIPELINE(rayTracingPipelineShaderGroupHandleCaptureReplayMixed)
                CASE_RAY_TRACING_PIPELINE(rayTracingPipelineTraceRaysIndirect)
                CASE_RAY_TRACING_PIPELINE(rayTraversalPrimitiveCulling)
        default:
                return "Unknown feature (index " + std::to_string(index)
                       + ") in VkPhysicalDeviceRayTracingPipelineFeaturesKHR)";
        }
}
}
