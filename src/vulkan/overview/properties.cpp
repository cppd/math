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

#include "properties.h"

#include "print.h"

#include <src/com/print.h>

#include <algorithm>

namespace ns::vulkan
{
namespace
{
template <typename T>
void add_value(
        const T& value,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, to_string(value));
}

void add_value(
        const VkPointClippingBehavior value,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, point_clipping_behavior_to_string(value));
}

void add_value(
        const VkShaderFloatControlsIndependence value,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, shader_float_controls_independence_to_string(value));
}

void add_sample(
        const VkSampleCountFlags flags,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, samples_to_string(flags));
}

void add_shader_stage(
        const VkShaderStageFlags flags,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, shader_stages_to_string(flags));
}

void add_subgroup_feature(
        const VkSubgroupFeatureFlags flags,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, subgroup_features_to_string(flags));
}

void add_resolve_mode(
        const VkResolveModeFlags flags,
        const std::string_view& name,
        std::vector<std::tuple<std::string, std::string>>* const properties)
{
        properties->emplace_back(name, resolve_modes_to_string(flags));
}
}

#define ADD_VALUE_10(v) add_value(device_properties.properties_10.limits.v, #v, &properties)

#define ADD_SAMPLE_10(v) add_sample(device_properties.properties_10.limits.v, #v, &properties)

#define ADD_VALUE_11(v) add_value(device_properties.properties_11.v, #v, &properties)

#define ADD_SHADER_STAGE_11(v) add_shader_stage(device_properties.properties_11.v, #v, &properties)

#define ADD_SUBGROUP_FEATURE_11(v) add_subgroup_feature(device_properties.properties_11.v, #v, &properties)

#define ADD_VALUE_12(v) add_value(device_properties.properties_12.v, #v, &properties)

#define ADD_SAMPLE_12(v) add_sample(device_properties.properties_12.v, #v, &properties)

#define ADD_RESOLVE_MODE_12(v) add_resolve_mode(device_properties.properties_12.v, #v, &properties)

std::vector<std::tuple<std::string, std::string>> find_properties(const PhysicalDeviceProperties& device_properties)
{
        std::vector<std::tuple<std::string, std::string>> properties;

        ADD_SAMPLE_10(framebufferColorSampleCounts);
        ADD_SAMPLE_10(framebufferDepthSampleCounts);
        ADD_SAMPLE_10(framebufferNoAttachmentsSampleCounts);
        ADD_SAMPLE_10(framebufferStencilSampleCounts);
        ADD_SAMPLE_10(sampledImageColorSampleCounts);
        ADD_SAMPLE_10(sampledImageDepthSampleCounts);
        ADD_SAMPLE_10(sampledImageIntegerSampleCounts);
        ADD_SAMPLE_10(sampledImageStencilSampleCounts);
        ADD_SAMPLE_10(storageImageSampleCounts);
        ADD_VALUE_10(bufferImageGranularity);
        ADD_VALUE_10(discreteQueuePriorities);
        ADD_VALUE_10(lineWidthGranularity);
        ADD_VALUE_10(lineWidthRange);
        ADD_VALUE_10(maxBoundDescriptorSets);
        ADD_VALUE_10(maxClipDistances);
        ADD_VALUE_10(maxColorAttachments);
        ADD_VALUE_10(maxCombinedClipAndCullDistances);
        ADD_VALUE_10(maxComputeSharedMemorySize);
        ADD_VALUE_10(maxComputeWorkGroupCount);
        ADD_VALUE_10(maxComputeWorkGroupInvocations);
        ADD_VALUE_10(maxComputeWorkGroupSize);
        ADD_VALUE_10(maxCullDistances);
        ADD_VALUE_10(maxDescriptorSetInputAttachments);
        ADD_VALUE_10(maxDescriptorSetSampledImages);
        ADD_VALUE_10(maxDescriptorSetSamplers);
        ADD_VALUE_10(maxDescriptorSetStorageBuffers);
        ADD_VALUE_10(maxDescriptorSetStorageBuffersDynamic);
        ADD_VALUE_10(maxDescriptorSetStorageImages);
        ADD_VALUE_10(maxDescriptorSetUniformBuffers);
        ADD_VALUE_10(maxDescriptorSetUniformBuffersDynamic);
        ADD_VALUE_10(maxDrawIndexedIndexValue);
        ADD_VALUE_10(maxDrawIndirectCount);
        ADD_VALUE_10(maxFragmentCombinedOutputResources);
        ADD_VALUE_10(maxFragmentDualSrcAttachments);
        ADD_VALUE_10(maxFragmentInputComponents);
        ADD_VALUE_10(maxFragmentOutputAttachments);
        ADD_VALUE_10(maxFramebufferHeight);
        ADD_VALUE_10(maxFramebufferLayers);
        ADD_VALUE_10(maxFramebufferWidth);
        ADD_VALUE_10(maxGeometryInputComponents);
        ADD_VALUE_10(maxGeometryOutputComponents);
        ADD_VALUE_10(maxGeometryOutputVertices);
        ADD_VALUE_10(maxGeometryShaderInvocations);
        ADD_VALUE_10(maxGeometryTotalOutputComponents);
        ADD_VALUE_10(maxImageArrayLayers);
        ADD_VALUE_10(maxImageDimension1D);
        ADD_VALUE_10(maxImageDimension2D);
        ADD_VALUE_10(maxImageDimension3D);
        ADD_VALUE_10(maxImageDimensionCube);
        ADD_VALUE_10(maxInterpolationOffset);
        ADD_VALUE_10(maxMemoryAllocationCount);
        ADD_VALUE_10(maxPerStageDescriptorInputAttachments);
        ADD_VALUE_10(maxPerStageDescriptorSampledImages);
        ADD_VALUE_10(maxPerStageDescriptorSamplers);
        ADD_VALUE_10(maxPerStageDescriptorStorageBuffers);
        ADD_VALUE_10(maxPerStageDescriptorStorageImages);
        ADD_VALUE_10(maxPerStageDescriptorUniformBuffers);
        ADD_VALUE_10(maxPerStageResources);
        ADD_VALUE_10(maxPushConstantsSize);
        ADD_VALUE_10(maxSampleMaskWords);
        ADD_VALUE_10(maxSamplerAllocationCount);
        ADD_VALUE_10(maxSamplerAnisotropy);
        ADD_VALUE_10(maxSamplerLodBias);
        ADD_VALUE_10(maxStorageBufferRange);
        ADD_VALUE_10(maxTessellationControlPerPatchOutputComponents);
        ADD_VALUE_10(maxTessellationControlPerVertexInputComponents);
        ADD_VALUE_10(maxTessellationControlPerVertexOutputComponents);
        ADD_VALUE_10(maxTessellationControlTotalOutputComponents);
        ADD_VALUE_10(maxTessellationEvaluationInputComponents);
        ADD_VALUE_10(maxTessellationEvaluationOutputComponents);
        ADD_VALUE_10(maxTessellationGenerationLevel);
        ADD_VALUE_10(maxTessellationPatchSize);
        ADD_VALUE_10(maxTexelBufferElements);
        ADD_VALUE_10(maxTexelGatherOffset);
        ADD_VALUE_10(maxTexelOffset);
        ADD_VALUE_10(maxUniformBufferRange);
        ADD_VALUE_10(maxVertexInputAttributeOffset);
        ADD_VALUE_10(maxVertexInputAttributes);
        ADD_VALUE_10(maxVertexInputBindingStride);
        ADD_VALUE_10(maxVertexInputBindings);
        ADD_VALUE_10(maxVertexOutputComponents);
        ADD_VALUE_10(maxViewportDimensions);
        ADD_VALUE_10(maxViewports);
        ADD_VALUE_10(minInterpolationOffset);
        ADD_VALUE_10(minMemoryMapAlignment);
        ADD_VALUE_10(minStorageBufferOffsetAlignment);
        ADD_VALUE_10(minTexelBufferOffsetAlignment);
        ADD_VALUE_10(minTexelGatherOffset);
        ADD_VALUE_10(minTexelOffset);
        ADD_VALUE_10(minUniformBufferOffsetAlignment);
        ADD_VALUE_10(mipmapPrecisionBits);
        ADD_VALUE_10(nonCoherentAtomSize);
        ADD_VALUE_10(optimalBufferCopyOffsetAlignment);
        ADD_VALUE_10(optimalBufferCopyRowPitchAlignment);
        ADD_VALUE_10(pointSizeGranularity);
        ADD_VALUE_10(pointSizeRange);
        ADD_VALUE_10(sparseAddressSpaceSize);
        ADD_VALUE_10(standardSampleLocations);
        ADD_VALUE_10(strictLines);
        ADD_VALUE_10(subPixelInterpolationOffsetBits);
        ADD_VALUE_10(subPixelPrecisionBits);
        ADD_VALUE_10(subTexelPrecisionBits);
        ADD_VALUE_10(viewportBoundsRange);
        ADD_VALUE_10(viewportSubPixelBits);
        //
        ADD_VALUE_11(pointClippingBehavior);
        ADD_SHADER_STAGE_11(subgroupSupportedStages);
        ADD_SUBGROUP_FEATURE_11(subgroupSupportedOperations);
        ADD_VALUE_11(maxMemoryAllocationSize);
        ADD_VALUE_11(maxMultiviewInstanceIndex);
        ADD_VALUE_11(maxMultiviewViewCount);
        ADD_VALUE_11(maxPerSetDescriptors);
        ADD_VALUE_11(protectedNoFault);
        ADD_VALUE_11(subgroupQuadOperationsInAllStages);
        ADD_VALUE_11(subgroupSize);
        //
        ADD_RESOLVE_MODE_12(supportedDepthResolveModes);
        ADD_RESOLVE_MODE_12(supportedStencilResolveModes);
        ADD_SAMPLE_12(framebufferIntegerColorSampleCounts);
        ADD_VALUE_12(denormBehaviorIndependence);
        ADD_VALUE_12(roundingModeIndependence);
        ADD_VALUE_12(filterMinmaxImageComponentMapping);
        ADD_VALUE_12(filterMinmaxSingleComponentFormats);
        ADD_VALUE_12(independentResolve);
        ADD_VALUE_12(independentResolveNone);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindInputAttachments);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindSampledImages);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindSamplers);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindStorageBuffers);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindStorageBuffersDynamic);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindStorageImages);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindUniformBuffers);
        ADD_VALUE_12(maxDescriptorSetUpdateAfterBindUniformBuffersDynamic);
        ADD_VALUE_12(maxPerStageDescriptorUpdateAfterBindInputAttachments);
        ADD_VALUE_12(maxPerStageDescriptorUpdateAfterBindSampledImages);
        ADD_VALUE_12(maxPerStageDescriptorUpdateAfterBindSamplers);
        ADD_VALUE_12(maxPerStageDescriptorUpdateAfterBindStorageBuffers);
        ADD_VALUE_12(maxPerStageDescriptorUpdateAfterBindStorageImages);
        ADD_VALUE_12(maxPerStageDescriptorUpdateAfterBindUniformBuffers);
        ADD_VALUE_12(maxPerStageUpdateAfterBindResources);
        ADD_VALUE_12(maxTimelineSemaphoreValueDifference);
        ADD_VALUE_12(maxUpdateAfterBindDescriptorsInAllPools);
        ADD_VALUE_12(quadDivergentImplicitLod);
        ADD_VALUE_12(robustBufferAccessUpdateAfterBind);
        ADD_VALUE_12(shaderDenormFlushToZeroFloat16);
        ADD_VALUE_12(shaderDenormFlushToZeroFloat32);
        ADD_VALUE_12(shaderDenormFlushToZeroFloat64);
        ADD_VALUE_12(shaderDenormPreserveFloat16);
        ADD_VALUE_12(shaderDenormPreserveFloat32);
        ADD_VALUE_12(shaderDenormPreserveFloat64);
        ADD_VALUE_12(shaderInputAttachmentArrayNonUniformIndexingNative);
        ADD_VALUE_12(shaderRoundingModeRTEFloat16);
        ADD_VALUE_12(shaderRoundingModeRTEFloat32);
        ADD_VALUE_12(shaderRoundingModeRTEFloat64);
        ADD_VALUE_12(shaderRoundingModeRTZFloat16);
        ADD_VALUE_12(shaderRoundingModeRTZFloat32);
        ADD_VALUE_12(shaderRoundingModeRTZFloat64);
        ADD_VALUE_12(shaderSampledImageArrayNonUniformIndexingNative);
        ADD_VALUE_12(shaderSignedZeroInfNanPreserveFloat16);
        ADD_VALUE_12(shaderSignedZeroInfNanPreserveFloat32);
        ADD_VALUE_12(shaderSignedZeroInfNanPreserveFloat64);
        ADD_VALUE_12(shaderStorageBufferArrayNonUniformIndexingNative);
        ADD_VALUE_12(shaderStorageImageArrayNonUniformIndexingNative);
        ADD_VALUE_12(shaderUniformBufferArrayNonUniformIndexingNative);

        std::sort(
                properties.begin(), properties.end(),
                [](const auto& v1, const auto& v2)
                {
                        return std::get<0>(v1) < std::get<0>(v2);
                });

        return properties;
}
}
