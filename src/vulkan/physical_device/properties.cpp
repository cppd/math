/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "info.h"

#include <src/com/print.h>
#include <src/vulkan/strings.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace ns::vulkan::physical_device
{
namespace
{
std::vector<std::string> sorted(std::vector<std::string>&& s)
{
        std::ranges::sort(s);
        return s;
}

template <typename T>
void add_value(
        const T& value,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, std::vector{to_string(value)});
}

void add_value(
        const VkPointClippingBehavior value,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, std::vector{strings::point_clipping_behavior_to_string(value)});
}

void add_value(
        const VkShaderFloatControlsIndependence value,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, std::vector{strings::shader_float_controls_independence_to_string(value)});
}

void add_value(
        const VkPipelineRobustnessBufferBehavior value,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, std::vector{strings::pipeline_robustness_buffer_behavior_to_string(value)});
}

void add_value(
        const VkPipelineRobustnessImageBehavior value,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, std::vector{strings::pipeline_robustness_image_behavior_to_string(value)});
}

void add_image_layouts(
        const std::vector<VkImageLayout>& layouts,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        std::vector<std::string> s;
        s.reserve(layouts.size());
        for (const VkImageLayout layout : layouts)
        {
                s.push_back(strings::image_layout_to_string(layout));
        }
        strings->emplace_back(name, sorted(std::move(s)));
}

void add_sample_count(
        const VkSampleCountFlags flags,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, strings::sample_counts_to_strings(flags));
}

void add_shader_stage(
        const VkShaderStageFlags flags,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, sorted(strings::shader_stages_to_strings(flags)));
}

void add_subgroup_feature(
        const VkSubgroupFeatureFlags flags,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, sorted(strings::subgroup_features_to_strings(flags)));
}

void add_resolve_mode(
        const VkResolveModeFlags flags,
        const std::string_view name,
        std::vector<std::tuple<std::string, std::vector<std::string>>>* const strings)
{
        strings->emplace_back(name, sorted(strings::resolve_modes_to_strings(flags)));
}
}

#define ADD_VALUE_10(v) add_value(properties.properties_10.limits.v, "Limits::" #v, &strings)

#define ADD_SAMPLE_COUNT_10(v) add_sample_count(properties.properties_10.limits.v, "Limits::" #v, &strings)

#define ADD_VALUE_11(v) add_value(properties.properties_11.v, "Vulkan11::" #v, &strings)

#define ADD_SHADER_STAGE_11(v) add_shader_stage(properties.properties_11.v, "Vulkan11::" #v, &strings)

#define ADD_SUBGROUP_FEATURE_11(v) add_subgroup_feature(properties.properties_11.v, "Vulkan11::" #v, &strings)

#define ADD_VALUE_12(v) add_value(properties.properties_12.v, "Vulkan12::" #v, &strings)

#define ADD_SAMPLE_COUNT_12(v) add_sample_count(properties.properties_12.v, "Vulkan12::" #v, &strings)

#define ADD_RESOLVE_MODE_12(v) add_resolve_mode(properties.properties_12.v, "Vulkan12::" #v, &strings)

#define ADD_VALUE_13(v) add_value(properties.properties_13.v, "Vulkan13::" #v, &strings)

#define ADD_SHADER_STAGE_13(v) add_shader_stage(properties.properties_13.v, "Vulkan13::" #v, &strings)

#define ADD_VALUE_14(v) add_value(properties.properties_14.v, "Vulkan14::" #v, &strings)

#define ADD_COPY_SRC_LAYOUTS_14(v) add_image_layouts(properties.copy_src_layouts, "Vulkan14::copySrcLayouts", &strings)

#define ADD_COPY_DST_LAYOUTS_14(v) add_image_layouts(properties.copy_dst_layouts, "Vulkan14::copyDstLayouts", &strings)

#define ADD_VALUE_ACCELERATION_STRUCTURE(v) \
        add_value(properties.acceleration_structure->v, "AccelerationStructure::" #v, &strings)

#define ADD_VALUE_RAY_TRACING_PIPELINE(v) \
        add_value(properties.ray_tracing_pipeline->v, "RayTracingPipeline::" #v, &strings)

std::vector<std::tuple<std::string, std::vector<std::string>>> device_properties_to_strings(
        const Properties& properties)
{
        std::vector<std::tuple<std::string, std::vector<std::string>>> strings;

        ADD_SAMPLE_COUNT_10(framebufferColorSampleCounts);
        ADD_SAMPLE_COUNT_10(framebufferDepthSampleCounts);
        ADD_SAMPLE_COUNT_10(framebufferNoAttachmentsSampleCounts);
        ADD_SAMPLE_COUNT_10(framebufferStencilSampleCounts);
        ADD_SAMPLE_COUNT_10(sampledImageColorSampleCounts);
        ADD_SAMPLE_COUNT_10(sampledImageDepthSampleCounts);
        ADD_SAMPLE_COUNT_10(sampledImageIntegerSampleCounts);
        ADD_SAMPLE_COUNT_10(sampledImageStencilSampleCounts);
        ADD_SAMPLE_COUNT_10(storageImageSampleCounts);
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

        ADD_SHADER_STAGE_11(subgroupSupportedStages);
        ADD_SUBGROUP_FEATURE_11(subgroupSupportedOperations);
        ADD_VALUE_11(maxMemoryAllocationSize);
        ADD_VALUE_11(maxMultiviewInstanceIndex);
        ADD_VALUE_11(maxMultiviewViewCount);
        ADD_VALUE_11(maxPerSetDescriptors);
        ADD_VALUE_11(pointClippingBehavior);
        ADD_VALUE_11(protectedNoFault);
        ADD_VALUE_11(subgroupQuadOperationsInAllStages);
        ADD_VALUE_11(subgroupSize);

        ADD_RESOLVE_MODE_12(supportedDepthResolveModes);
        ADD_RESOLVE_MODE_12(supportedStencilResolveModes);
        ADD_SAMPLE_COUNT_12(framebufferIntegerColorSampleCounts);
        ADD_VALUE_12(denormBehaviorIndependence);
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
        ADD_VALUE_12(roundingModeIndependence);
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

        ADD_SHADER_STAGE_13(requiredSubgroupSizeStages);
        ADD_VALUE_13(integerDotProduct16BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProduct16BitSignedAccelerated);
        ADD_VALUE_13(integerDotProduct16BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProduct32BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProduct32BitSignedAccelerated);
        ADD_VALUE_13(integerDotProduct32BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProduct4x8BitPackedMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProduct4x8BitPackedSignedAccelerated);
        ADD_VALUE_13(integerDotProduct4x8BitPackedUnsignedAccelerated);
        ADD_VALUE_13(integerDotProduct64BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProduct64BitSignedAccelerated);
        ADD_VALUE_13(integerDotProduct64BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProduct8BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProduct8BitSignedAccelerated);
        ADD_VALUE_13(integerDotProduct8BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating16BitSignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating16BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating32BitSignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating32BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating64BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating64BitSignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating64BitUnsignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating8BitSignedAccelerated);
        ADD_VALUE_13(integerDotProductAccumulatingSaturating8BitUnsignedAccelerated);
        ADD_VALUE_13(maxBufferSize);
        ADD_VALUE_13(maxComputeWorkgroupSubgroups);
        ADD_VALUE_13(maxDescriptorSetInlineUniformBlocks);
        ADD_VALUE_13(maxDescriptorSetUpdateAfterBindInlineUniformBlocks);
        ADD_VALUE_13(maxInlineUniformBlockSize);
        ADD_VALUE_13(maxInlineUniformTotalSize);
        ADD_VALUE_13(maxPerStageDescriptorInlineUniformBlocks);
        ADD_VALUE_13(maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks);
        ADD_VALUE_13(maxSubgroupSize);
        ADD_VALUE_13(minSubgroupSize);
        ADD_VALUE_13(storageTexelBufferOffsetAlignmentBytes);
        ADD_VALUE_13(storageTexelBufferOffsetSingleTexelAlignment);
        ADD_VALUE_13(uniformTexelBufferOffsetAlignmentBytes);
        ADD_VALUE_13(uniformTexelBufferOffsetSingleTexelAlignment);

        ADD_VALUE_14(lineSubPixelPrecisionBits);
        ADD_VALUE_14(maxVertexAttribDivisor);
        ADD_VALUE_14(supportsNonZeroFirstInstance);
        ADD_VALUE_14(maxPushDescriptors);
        ADD_VALUE_14(dynamicRenderingLocalReadDepthStencilAttachments);
        ADD_VALUE_14(dynamicRenderingLocalReadMultisampledAttachments);
        ADD_VALUE_14(earlyFragmentMultisampleCoverageAfterSampleCounting);
        ADD_VALUE_14(earlyFragmentSampleMaskTestBeforeSampleCounting);
        ADD_VALUE_14(depthStencilSwizzleOneSupport);
        ADD_VALUE_14(polygonModePointSize);
        ADD_VALUE_14(nonStrictSinglePixelWideLinesUseParallelogram);
        ADD_VALUE_14(nonStrictWideLinesUseParallelogram);
        ADD_VALUE_14(blockTexelViewCompatibleMultipleLayers);
        ADD_VALUE_14(maxCombinedImageSamplerDescriptorCount);
        ADD_VALUE_14(fragmentShadingRateClampCombinerInputs);
        ADD_VALUE_14(defaultRobustnessStorageBuffers);
        ADD_VALUE_14(defaultRobustnessUniformBuffers);
        ADD_VALUE_14(defaultRobustnessVertexInputs);
        ADD_VALUE_14(defaultRobustnessImages);
        ADD_COPY_SRC_LAYOUTS_14(v);
        ADD_COPY_DST_LAYOUTS_14(v);
        ADD_VALUE_14(identicalMemoryTypeRequirements);

        if (properties.acceleration_structure)
        {
                ADD_VALUE_ACCELERATION_STRUCTURE(maxDescriptorSetAccelerationStructures);
                ADD_VALUE_ACCELERATION_STRUCTURE(maxDescriptorSetUpdateAfterBindAccelerationStructures);
                ADD_VALUE_ACCELERATION_STRUCTURE(maxGeometryCount);
                ADD_VALUE_ACCELERATION_STRUCTURE(maxInstanceCount);
                ADD_VALUE_ACCELERATION_STRUCTURE(maxPerStageDescriptorAccelerationStructures);
                ADD_VALUE_ACCELERATION_STRUCTURE(maxPerStageDescriptorUpdateAfterBindAccelerationStructures);
                ADD_VALUE_ACCELERATION_STRUCTURE(maxPrimitiveCount);
                ADD_VALUE_ACCELERATION_STRUCTURE(minAccelerationStructureScratchOffsetAlignment);
        }

        if (properties.ray_tracing_pipeline)
        {
                ADD_VALUE_RAY_TRACING_PIPELINE(maxRayDispatchInvocationCount);
                ADD_VALUE_RAY_TRACING_PIPELINE(maxRayHitAttributeSize);
                ADD_VALUE_RAY_TRACING_PIPELINE(maxRayRecursionDepth);
                ADD_VALUE_RAY_TRACING_PIPELINE(maxShaderGroupStride);
                ADD_VALUE_RAY_TRACING_PIPELINE(shaderGroupBaseAlignment);
                ADD_VALUE_RAY_TRACING_PIPELINE(shaderGroupHandleAlignment);
                ADD_VALUE_RAY_TRACING_PIPELINE(shaderGroupHandleCaptureReplaySize);
                ADD_VALUE_RAY_TRACING_PIPELINE(shaderGroupHandleSize);
        }

        std::ranges::sort(
                strings,
                [](const auto& v1, const auto& v2)
                {
                        return std::get<0>(v1) < std::get<0>(v2);
                });

        return strings;
}
}
