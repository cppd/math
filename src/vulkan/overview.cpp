/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "overview.h"

#include "device.h"
#include "error.h"
#include "print.h"
#include "query.h"

#include <src/com/print.h>
#include <src/com/string_tree.h>
#include <src/window/manage.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace vulkan
{
namespace
{
constexpr unsigned TREE_LEVEL_INDENT = 2;

template <typename T>
std::vector<std::string> sorted(const T& s)
{
        std::vector<std::string> res(s.cbegin(), s.cend());
        std::sort(res.begin(), res.end());
        return res;
}

template <class T>
T value(const T& v)
{
        return v;
}

template <class T, std::size_t N>
std::vector<T> value(const T (&data)[N])
{
        static_assert(N > 0);
        std::vector<T> r;
        r.reserve(N);
        for (size_t i = 0; i < N; ++i)
        {
                r.push_back(data[i]);
        }
        return r;
}

#define ADD_FLAG(s, flags, flag, name)            \
        do                                        \
        {                                         \
                if (((flags) & (flag)) == (flag)) \
                {                                 \
                        if (!(s).empty())         \
                        {                         \
                                (s) += ", ";      \
                        }                         \
                        (s) += (name);            \
                        (flags) &= ~(flag);       \
                }                                 \
        } while (false)

#define ADD_FLAG_UNKNOWN(s, flags)                              \
        do                                                      \
        {                                                       \
                if ((flags) != 0)                               \
                {                                               \
                        (s) += (s).empty() ? "" : ", ";         \
                        (s) += "UNKNOWN (";                     \
                        (s) += to_string_binary((flags), "0b"); \
                        (s) += ")";                             \
                }                                               \
        } while (false)

std::string samples(VkSampleCountFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_1_BIT, "1");
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_2_BIT, "2");
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_4_BIT, "4");
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_8_BIT, "8");
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_16_BIT, "16");
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_32_BIT, "32");
        ADD_FLAG(s, flags, VK_SAMPLE_COUNT_64_BIT, "64");
        ADD_FLAG_UNKNOWN(s, flags);
        return s;
}

std::string resolve_modes(VkResolveModeFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        ADD_FLAG(s, flags, VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, "SAMPLE_ZERO");
        ADD_FLAG(s, flags, VK_RESOLVE_MODE_AVERAGE_BIT, "AVERAGE");
        ADD_FLAG(s, flags, VK_RESOLVE_MODE_MIN_BIT, "MIN");
        ADD_FLAG(s, flags, VK_RESOLVE_MODE_MAX_BIT, "MAX");
        ADD_FLAG_UNKNOWN(s, flags);
        return s;
}

std::string shader_stages(VkShaderStageFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        ADD_FLAG(s, flags, VK_SHADER_STAGE_VERTEX_BIT, "VERTEX");
        ADD_FLAG(s, flags, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TESSELLATION_CONTROL");
        ADD_FLAG(s, flags, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TESSELLATION_EVALUATION");
        ADD_FLAG(s, flags, VK_SHADER_STAGE_GEOMETRY_BIT, "GEOMETRY");
        ADD_FLAG(s, flags, VK_SHADER_STAGE_FRAGMENT_BIT, "FRAGMENT");
        ADD_FLAG(s, flags, VK_SHADER_STAGE_COMPUTE_BIT, "COMPUTE");
        ADD_FLAG_UNKNOWN(s, flags);
        return s;
}

std::string subgroup_features(VkSubgroupFeatureFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_BASIC_BIT, "BASIC");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_VOTE_BIT, "VOTE");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_ARITHMETIC_BIT, "ARITHMETIC");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_BALLOT_BIT, "BALLOT");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_SHUFFLE_BIT, "SHUFFLE");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT, "SHUFFLE_RELATIVE");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_CLUSTERED_BIT, "CLUSTERED");
        ADD_FLAG(s, flags, VK_SUBGROUP_FEATURE_QUAD_BIT, "QUAD");
        ADD_FLAG_UNKNOWN(s, flags);
        return s;
}

std::string shader_float_controls_independence(VkShaderFloatControlsIndependence v)
{
        if (v == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY)
        {
                return "32_BIT_ONLY";
        }

        if (v == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL)
        {
                return "ALL";
        }
        if (v == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE)
        {
                return "NONE";
        }
        return "UNKNOWN";
}

std::string point_clipping_behavior(VkPointClippingBehavior v)
{
        if (v == VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES)
        {
                return "ALL_CLIP_PLANES";
        }
        if (v == VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY)
        {
                return "USER_CLIP_PLANES_ONLY";
        }
        return "UNKNOWN";
}

void conformance_version(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        std::ostringstream oss;
        VkConformanceVersion v = device.properties().properties_12.conformanceVersion;
        oss << static_cast<int>(v.major) << "." << static_cast<int>(v.minor) << "." << static_cast<int>(v.subminor)
            << "." << static_cast<int>(v.patch);
        size_t node = tree->add(device_node, "Conformance Version");
        tree->add(node, oss.str());
}

void device_name(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t type_node = tree->add(device_node, "Device Name");
        tree->add(type_node, device.properties().properties_10.deviceName);
}

void driver_info(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t node = tree->add(device_node, "Driver");
        tree->add(node, std::string("Name = ") + device.properties().properties_12.driverName);
        tree->add(node, std::string("Info = ") + device.properties().properties_12.driverInfo);
}

void device_type(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t type_node = tree->add(device_node, "Device Type");
        try
        {
                tree->add(type_node, physical_device_type_to_string(device.properties().properties_10.deviceType));
        }
        catch (const std::exception& e)
        {
                tree->add(type_node, e.what());
        }
}

void api_version(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t api_node = tree->add(device_node, "API Version");
        try
        {
                tree->add(api_node, api_version_to_string(device.properties().properties_10.apiVersion));
        }
        catch (const std::exception& e)
        {
                tree->add(api_node, e.what());
        }
}

void extensions(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t extensions_node = tree->add(device_node, "Extensions");

        try
        {
                for (const std::string& e : sorted(device.supported_extensions()))
                {
                        tree->add(extensions_node, e);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(extensions_node, e.what());
        }
}

#define ADD_VALUE_10(v) properties.emplace_back(#v, to_string(value(device.properties().properties_10.limits.v)))
#define ADD_SAMPLE_10(v) properties.emplace_back(#v, samples(device.properties().properties_10.limits.v))

#define ADD_VALUE_11(v) properties.emplace_back(#v, to_string(value(device.properties().properties_11.v)))
#define ADD_SHADER_STAGE_11(v) properties.emplace_back(#v, shader_stages(device.properties().properties_11.v))
#define ADD_POINT_CLIPPING_BEHAVIOR_11(v) \
        properties.emplace_back(#v, point_clipping_behavior(device.properties().properties_11.v))
#define ADD_SUBGROUP_FEATURE_11(v) properties.emplace_back(#v, subgroup_features(device.properties().properties_11.v))

#define ADD_VALUE_12(v) properties.emplace_back(#v, to_string(value(device.properties().properties_12.v)))
#define ADD_SAMPLE_12(v) properties.emplace_back(#v, samples(device.properties().properties_12.v))
#define ADD_RESOLVE_MODE_12(v) properties.emplace_back(#v, resolve_modes(device.properties().properties_12.v))
#define ADD_SHADER_FLOAT_CONTROLS_INDEPENDENCE_12(v) \
        properties.emplace_back(#v, shader_float_controls_independence(device.properties().properties_12.v))

void properties(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t properties_node = tree->add(device_node, "Properties");

        try
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
                ADD_POINT_CLIPPING_BEHAVIOR_11(pointClippingBehavior);
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
                ADD_SHADER_FLOAT_CONTROLS_INDEPENDENCE_12(denormBehaviorIndependence);
                ADD_SHADER_FLOAT_CONTROLS_INDEPENDENCE_12(roundingModeIndependence);
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

                for (const auto& [name, value] : properties)
                {
                        tree->add(properties_node, name + " = " + value);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(properties_node, e.what());
        }
}

#define ADD_FEATURE_10(v)                                       \
        do                                                      \
        {                                                       \
                if (device.features().features_10.v == VK_TRUE) \
                {                                               \
                        supported.emplace_back(#v);             \
                }                                               \
                else                                            \
                {                                               \
                        not_supported.emplace_back(#v);         \
                }                                               \
        } while (false)

#define ADD_FEATURE_11(v)                                       \
        do                                                      \
        {                                                       \
                if (device.features().features_11.v == VK_TRUE) \
                {                                               \
                        supported.emplace_back(#v);             \
                }                                               \
                else                                            \
                {                                               \
                        not_supported.emplace_back(#v);         \
                }                                               \
        } while (false)

#define ADD_FEATURE_12(v)                                       \
        do                                                      \
        {                                                       \
                if (device.features().features_12.v == VK_TRUE) \
                {                                               \
                        supported.emplace_back(#v);             \
                }                                               \
                else                                            \
                {                                               \
                        not_supported.emplace_back(#v);         \
                }                                               \
        } while (false)

void features(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t features_node = tree->add(device_node, "Features");
        size_t supported_node = tree->add(features_node, "Supported");
        size_t not_supported_node = tree->add(features_node, "Not Supported");

        try
        {
                std::vector<std::string> supported;
                std::vector<std::string> not_supported;

                ADD_FEATURE_10(alphaToOne);
                ADD_FEATURE_10(depthBiasClamp);
                ADD_FEATURE_10(depthBounds);
                ADD_FEATURE_10(depthClamp);
                ADD_FEATURE_10(drawIndirectFirstInstance);
                ADD_FEATURE_10(dualSrcBlend);
                ADD_FEATURE_10(fillModeNonSolid);
                ADD_FEATURE_10(fragmentStoresAndAtomics);
                ADD_FEATURE_10(fullDrawIndexUint32);
                ADD_FEATURE_10(geometryShader);
                ADD_FEATURE_10(imageCubeArray);
                ADD_FEATURE_10(independentBlend);
                ADD_FEATURE_10(inheritedQueries);
                ADD_FEATURE_10(largePoints);
                ADD_FEATURE_10(logicOp);
                ADD_FEATURE_10(multiDrawIndirect);
                ADD_FEATURE_10(multiViewport);
                ADD_FEATURE_10(occlusionQueryPrecise);
                ADD_FEATURE_10(pipelineStatisticsQuery);
                ADD_FEATURE_10(robustBufferAccess);
                ADD_FEATURE_10(sampleRateShading);
                ADD_FEATURE_10(samplerAnisotropy);
                ADD_FEATURE_10(shaderClipDistance);
                ADD_FEATURE_10(shaderCullDistance);
                ADD_FEATURE_10(shaderFloat64);
                ADD_FEATURE_10(shaderImageGatherExtended);
                ADD_FEATURE_10(shaderInt16);
                ADD_FEATURE_10(shaderInt64);
                ADD_FEATURE_10(shaderResourceMinLod);
                ADD_FEATURE_10(shaderResourceResidency);
                ADD_FEATURE_10(shaderSampledImageArrayDynamicIndexing);
                ADD_FEATURE_10(shaderStorageBufferArrayDynamicIndexing);
                ADD_FEATURE_10(shaderStorageImageArrayDynamicIndexing);
                ADD_FEATURE_10(shaderStorageImageExtendedFormats);
                ADD_FEATURE_10(shaderStorageImageMultisample);
                ADD_FEATURE_10(shaderStorageImageReadWithoutFormat);
                ADD_FEATURE_10(shaderStorageImageWriteWithoutFormat);
                ADD_FEATURE_10(shaderTessellationAndGeometryPointSize);
                ADD_FEATURE_10(shaderUniformBufferArrayDynamicIndexing);
                ADD_FEATURE_10(sparseBinding);
                ADD_FEATURE_10(sparseResidency16Samples);
                ADD_FEATURE_10(sparseResidency2Samples);
                ADD_FEATURE_10(sparseResidency4Samples);
                ADD_FEATURE_10(sparseResidency8Samples);
                ADD_FEATURE_10(sparseResidencyAliased);
                ADD_FEATURE_10(sparseResidencyBuffer);
                ADD_FEATURE_10(sparseResidencyImage2D);
                ADD_FEATURE_10(sparseResidencyImage3D);
                ADD_FEATURE_10(tessellationShader);
                ADD_FEATURE_10(textureCompressionASTC_LDR);
                ADD_FEATURE_10(textureCompressionBC);
                ADD_FEATURE_10(textureCompressionETC2);
                ADD_FEATURE_10(variableMultisampleRate);
                ADD_FEATURE_10(vertexPipelineStoresAndAtomics);
                ADD_FEATURE_10(wideLines);
                //
                ADD_FEATURE_11(multiview);
                ADD_FEATURE_11(multiviewGeometryShader);
                ADD_FEATURE_11(multiviewTessellationShader);
                ADD_FEATURE_11(protectedMemory);
                ADD_FEATURE_11(samplerYcbcrConversion);
                ADD_FEATURE_11(shaderDrawParameters);
                ADD_FEATURE_11(storageBuffer16BitAccess);
                ADD_FEATURE_11(storageInputOutput16);
                ADD_FEATURE_11(storagePushConstant16);
                ADD_FEATURE_11(uniformAndStorageBuffer16BitAccess);
                ADD_FEATURE_11(variablePointers);
                ADD_FEATURE_11(variablePointersStorageBuffer);
                //
                ADD_FEATURE_12(bufferDeviceAddress);
                ADD_FEATURE_12(bufferDeviceAddressCaptureReplay);
                ADD_FEATURE_12(bufferDeviceAddressMultiDevice);
                ADD_FEATURE_12(descriptorBindingPartiallyBound);
                ADD_FEATURE_12(descriptorBindingSampledImageUpdateAfterBind);
                ADD_FEATURE_12(descriptorBindingStorageBufferUpdateAfterBind);
                ADD_FEATURE_12(descriptorBindingStorageImageUpdateAfterBind);
                ADD_FEATURE_12(descriptorBindingStorageTexelBufferUpdateAfterBind);
                ADD_FEATURE_12(descriptorBindingUniformBufferUpdateAfterBind);
                ADD_FEATURE_12(descriptorBindingUniformTexelBufferUpdateAfterBind);
                ADD_FEATURE_12(descriptorBindingUpdateUnusedWhilePending);
                ADD_FEATURE_12(descriptorBindingVariableDescriptorCount);
                ADD_FEATURE_12(descriptorIndexing);
                ADD_FEATURE_12(drawIndirectCount);
                ADD_FEATURE_12(hostQueryReset);
                ADD_FEATURE_12(imagelessFramebuffer);
                ADD_FEATURE_12(runtimeDescriptorArray);
                ADD_FEATURE_12(samplerFilterMinmax);
                ADD_FEATURE_12(samplerMirrorClampToEdge);
                ADD_FEATURE_12(scalarBlockLayout);
                ADD_FEATURE_12(separateDepthStencilLayouts);
                ADD_FEATURE_12(shaderBufferInt64Atomics);
                ADD_FEATURE_12(shaderFloat16);
                ADD_FEATURE_12(shaderInputAttachmentArrayDynamicIndexing);
                ADD_FEATURE_12(shaderInputAttachmentArrayNonUniformIndexing);
                ADD_FEATURE_12(shaderInt8);
                ADD_FEATURE_12(shaderOutputLayer);
                ADD_FEATURE_12(shaderOutputViewportIndex);
                ADD_FEATURE_12(shaderSampledImageArrayNonUniformIndexing);
                ADD_FEATURE_12(shaderSharedInt64Atomics);
                ADD_FEATURE_12(shaderStorageBufferArrayNonUniformIndexing);
                ADD_FEATURE_12(shaderStorageImageArrayNonUniformIndexing);
                ADD_FEATURE_12(shaderStorageTexelBufferArrayDynamicIndexing);
                ADD_FEATURE_12(shaderStorageTexelBufferArrayNonUniformIndexing);
                ADD_FEATURE_12(shaderSubgroupExtendedTypes);
                ADD_FEATURE_12(shaderUniformBufferArrayNonUniformIndexing);
                ADD_FEATURE_12(shaderUniformTexelBufferArrayDynamicIndexing);
                ADD_FEATURE_12(shaderUniformTexelBufferArrayNonUniformIndexing);
                ADD_FEATURE_12(storageBuffer8BitAccess);
                ADD_FEATURE_12(storagePushConstant8);
                ADD_FEATURE_12(subgroupBroadcastDynamicId);
                ADD_FEATURE_12(timelineSemaphore);
                ADD_FEATURE_12(uniformAndStorageBuffer8BitAccess);
                ADD_FEATURE_12(uniformBufferStandardLayout);
                ADD_FEATURE_12(vulkanMemoryModel);
                ADD_FEATURE_12(vulkanMemoryModelAvailabilityVisibilityChains);
                ADD_FEATURE_12(vulkanMemoryModelDeviceScope);

                for (const std::string& name : sorted(supported))
                {
                        tree->add(supported_node, name);
                }

                for (const std::string& name : sorted(not_supported))
                {
                        tree->add(not_supported_node, name);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(features_node, e.what());
        }
}

void queues(
        const PhysicalDevice& device,
        const VkQueueFamilyProperties& family_properties,
        size_t family_index,
        size_t queue_families_node,
        StringTree* tree)
{
        size_t queue_family_node = tree->add(queue_families_node, "Family " + to_string(family_index));

        try
        {
                tree->add(queue_family_node, "queue count: " + to_string(family_properties.queueCount));

                if (family_properties.queueCount < 1)
                {
                        return;
                }

                if (family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                        tree->add(queue_family_node, "graphics");
                }
                if (family_properties.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                        tree->add(queue_family_node, "compute");
                }
                if (family_properties.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                        tree->add(queue_family_node, "transfer");
                }
                if (family_properties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                        tree->add(queue_family_node, "sparse binding");
                }
                if (family_properties.queueFlags & VK_QUEUE_PROTECTED_BIT)
                {
                        tree->add(queue_family_node, "protected");
                }

                if (device.queue_family_supports_presentation(family_index))
                {
                        tree->add(queue_family_node, "presentation");
                }
        }
        catch (const std::exception& e)
        {
                tree->add(queue_family_node, e.what());
        }
}

void queue_families(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t queue_families_node = tree->add(device_node, "QueueFamilies");

        try
        {
                for (size_t index = 0; const VkQueueFamilyProperties& properties : device.queue_families())
                {
                        queues(device, properties, index++, queue_families_node, tree);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(queue_families_node, e.what());
        }
}

//

void api_version(StringTree* tree)
{
        size_t api_node = tree->add("API Version");
        try
        {
                tree->add(api_node, api_version_to_string(supported_instance_api_version()));
        }
        catch (const std::exception& e)
        {
                tree->add(api_node, e.what());
        }
}
void extensions(StringTree* tree)
{
        size_t extensions_node = tree->add("Extensions");
        try
        {
                for (const std::string& extension : sorted(supported_instance_extensions()))
                {
                        tree->add(extensions_node, extension);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(extensions_node, e.what());
        }
}

void validation_layers(StringTree* tree)
{
        size_t validation_layers_node = tree->add("Validation Layers");
        try
        {
                for (const std::string& layer : sorted(supported_validation_layers()))
                {
                        tree->add(validation_layers_node, layer);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(validation_layers_node, e.what());
        }
}

void required_surface_extensions(StringTree* tree)
{
        size_t required_surface_extensions_node = tree->add("Required Surface Extensions");
        try
        {
                for (const std::string& extension : sorted(vulkan_create_surface_extensions()))
                {
                        tree->add(required_surface_extensions_node, extension);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(required_surface_extensions_node, e.what());
        }
}
}

std::string overview()
{
        StringTree tree;

        api_version(&tree);
        extensions(&tree);
        validation_layers(&tree);
        required_surface_extensions(&tree);

        return tree.text(TREE_LEVEL_INDENT);
}

std::string overview_physical_devices(VkInstance instance, VkSurfaceKHR surface)
{
        StringTree tree;

        std::unordered_set<std::string> uuids;

        for (const VkPhysicalDevice& d : physical_devices(instance))
        {
                PhysicalDevice device(d, surface);

                if (!uuids.emplace(to_string(value(device.properties().properties_10.pipelineCacheUUID))).second)
                {
                        continue;
                }

                size_t node = tree.add("Physical Device");

                device_name(device, node, &tree);
                device_type(device, node, &tree);
                api_version(device, node, &tree);
                driver_info(device, node, &tree);
                conformance_version(device, node, &tree);
                extensions(device, node, &tree);
                features(device, node, &tree);
                properties(device, node, &tree);
                queue_families(device, node, &tree);
        }

        return tree.text(TREE_LEVEL_INDENT);
}
}
