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

#include "features.h"

#include <src/com/error.h>

#include <cstring>
#include <string>

namespace ns::vulkan
{
namespace
{
constexpr std::size_t SIZE = sizeof(VkBool32);
constexpr VkBool32 TRUE = VK_TRUE;

template <typename T>
constexpr bool check_size(const std::size_t size)
{
        return ((size + alignof(T) - 1) / alignof(T)) * alignof(T) == sizeof(T);
}

template <typename T>
struct FeatureProperties;

template <>
struct FeatureProperties<VkPhysicalDeviceFeatures>
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceFeatures, robustBufferAccess);
        static constexpr std::size_t COUNT = 55;
        static std::string name(std::size_t index);
        static_assert(check_size<VkPhysicalDeviceFeatures>(COUNT * SIZE + OFFSET));
};

template <>
struct FeatureProperties<VkPhysicalDeviceVulkan11Features>
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceVulkan11Features, storageBuffer16BitAccess);
        static constexpr std::size_t COUNT = 12;
        static std::string name(std::size_t index);
        static_assert(check_size<VkPhysicalDeviceVulkan11Features>(COUNT * SIZE + OFFSET));
};

template <>
struct FeatureProperties<VkPhysicalDeviceVulkan12Features>
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceVulkan12Features, samplerMirrorClampToEdge);
        static constexpr std::size_t COUNT = 47;
        static std::string name(std::size_t index);
        static_assert(check_size<VkPhysicalDeviceVulkan12Features>(COUNT * SIZE + OFFSET));
};

template <>
struct FeatureProperties<VkPhysicalDeviceAccelerationStructureFeaturesKHR>
{
        static constexpr std::size_t OFFSET =
                offsetof(VkPhysicalDeviceAccelerationStructureFeaturesKHR, accelerationStructure);
        static constexpr std::size_t COUNT = 5;
        static std::string name(std::size_t index);
        static_assert(check_size<VkPhysicalDeviceAccelerationStructureFeaturesKHR>(COUNT * SIZE + OFFSET));
};

template <>
struct FeatureProperties<VkPhysicalDeviceRayQueryFeaturesKHR>
{
        static constexpr std::size_t OFFSET = offsetof(VkPhysicalDeviceRayQueryFeaturesKHR, rayQuery);
        static constexpr std::size_t COUNT = 1;
        static std::string name(std::size_t index);
        static_assert(check_size<VkPhysicalDeviceRayQueryFeaturesKHR>(COUNT * SIZE + OFFSET));
};

template <>
struct FeatureProperties<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>
{
        static constexpr std::size_t OFFSET =
                offsetof(VkPhysicalDeviceRayTracingPipelineFeaturesKHR, rayTracingPipeline);
        static constexpr std::size_t COUNT = 5;
        static std::string name(std::size_t index);
        static_assert(check_size<VkPhysicalDeviceRayTracingPipelineFeaturesKHR>(COUNT * SIZE + OFFSET));
};

#define CASE_FEATURE(name, type, feature)                                        \
        case (offsetof(type, feature) - FeatureProperties<type>::OFFSET) / SIZE: \
                return name "::" #feature;

#define CASE_FEATURE_10(feature) CASE_FEATURE("Features", VkPhysicalDeviceFeatures, feature)

#define CASE_FEATURE_11(feature) CASE_FEATURE("Vulkan11Features", VkPhysicalDeviceVulkan11Features, feature)

#define CASE_FEATURE_12(feature) CASE_FEATURE("Vulkan12Features", VkPhysicalDeviceVulkan12Features, feature)

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

class FeatureIsNotSupported final : public std::exception
{
        std::string text_;

public:
        explicit FeatureIsNotSupported(std::string&& name) noexcept : text_(std::move(name))
        {
        }

        const char* what() const noexcept override
        {
                return text_.c_str();
        }
};

template <typename T>
[[noreturn]] void feature_is_not_supported_error(T&& name)
{
        throw FeatureIsNotSupported(std::string(std::forward<T>(name)));
}

template <typename Features>
void add_features(Features* const dst, const Features& src)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        std::byte* dst_ptr = reinterpret_cast<std::byte*>(dst) + OFFSET;
        const std::byte* src_ptr = reinterpret_cast<const std::byte*>(&src) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, dst_ptr += SIZE, src_ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, src_ptr, SIZE);
                if (!feature)
                {
                        continue;
                }
                std::memcpy(dst_ptr, &TRUE, SIZE);
        }
}

template <bool REQUIRED, typename Features>
void set_features(const Features& features, const Features& supported, Features* const result)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* ptr = reinterpret_cast<const std::byte*>(&features) + OFFSET;
        const std::byte* supported_ptr = reinterpret_cast<const std::byte*>(&supported) + OFFSET;
        std::byte* result_ptr = reinterpret_cast<std::byte*>(result) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, ptr += SIZE, supported_ptr += SIZE, result_ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, ptr, SIZE);
                if (!feature)
                {
                        continue;
                }

                VkBool32 supported_feature;
                std::memcpy(&supported_feature, supported_ptr, SIZE);
                if (supported_feature)
                {
                        std::memcpy(result_ptr, &TRUE, SIZE);
                }
                else if (REQUIRED)
                {
                        feature_is_not_supported_error(FeatureProperties<Features>::name(i));
                }
        }
}

template <typename Features>
void check_features(const Features& required, const Features& supported)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* required_ptr = reinterpret_cast<const std::byte*>(&required) + OFFSET;
        const std::byte* supported_ptr = reinterpret_cast<const std::byte*>(&supported) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, required_ptr += SIZE, supported_ptr += SIZE)
        {
                VkBool32 required_feature;
                std::memcpy(&required_feature, required_ptr, SIZE);
                if (!required_feature)
                {
                        continue;
                }

                VkBool32 supported_feature;
                std::memcpy(&supported_feature, supported_ptr, SIZE);
                if (!supported_feature)
                {
                        feature_is_not_supported_error(FeatureProperties<Features>::name(i));
                }
        }
}

template <typename Features>
void features_to_strings(const Features& features, const bool enabled, std::vector<std::string>* const strings)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* ptr = reinterpret_cast<const std::byte*>(&features) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, ptr, SIZE);
                if (static_cast<bool>(feature) == enabled)
                {
                        strings->push_back(FeatureProperties<Features>::name(i));
                }
        }
}

template <bool REQUIRED>
void set_features(
        const PhysicalDeviceFeatures& features,
        const PhysicalDeviceFeatures& supported,
        PhysicalDeviceFeatures* const result)
{
        set_features<REQUIRED>(features.features_10, supported.features_10, &result->features_10);

        set_features<REQUIRED>(features.features_11, supported.features_11, &result->features_11);

        set_features<REQUIRED>(features.features_12, supported.features_12, &result->features_12);

        set_features<REQUIRED>(
                features.acceleration_structure, supported.acceleration_structure, &result->acceleration_structure);

        set_features<REQUIRED>(features.ray_query, supported.ray_query, &result->ray_query);

        set_features<REQUIRED>(
                features.ray_tracing_pipeline, supported.ray_tracing_pipeline, &result->ray_tracing_pipeline);
}
}

void add_features(PhysicalDeviceFeatures* const dst, const PhysicalDeviceFeatures& src)
{
        add_features(&dst->features_10, src.features_10);
        add_features(&dst->features_11, src.features_11);
        add_features(&dst->features_12, src.features_12);
        add_features(&dst->acceleration_structure, src.acceleration_structure);
        add_features(&dst->ray_query, src.ray_query);
        add_features(&dst->ray_tracing_pipeline, src.ray_tracing_pipeline);
}

PhysicalDeviceFeatures make_features(
        const PhysicalDeviceFeatures& required,
        const PhysicalDeviceFeatures& optional,
        const PhysicalDeviceFeatures& supported)
{
        PhysicalDeviceFeatures result_features = {};

        try
        {
                set_features<true>(required, supported, &result_features);
        }
        catch (const FeatureIsNotSupported& e)
        {
                error(std::string("Required physical device feature ") + e.what() + " is not supported");
        }

        try
        {
                set_features<false>(optional, supported, &result_features);
        }
        catch (const FeatureIsNotSupported&)
        {
                error("Exception when setting optional device features");
        }

        return result_features;
}

bool check_features(const PhysicalDeviceFeatures& required, const PhysicalDeviceFeatures& supported)
{
        try
        {
                check_features(required.features_10, supported.features_10);
                check_features(required.features_11, supported.features_11);
                check_features(required.features_12, supported.features_12);
                check_features(required.acceleration_structure, supported.acceleration_structure);
                check_features(required.ray_query, supported.ray_query);
                check_features(required.ray_tracing_pipeline, supported.ray_tracing_pipeline);
        }
        catch (const FeatureIsNotSupported&)
        {
                return false;
        }
        return true;
}

std::vector<std::string> features_to_strings(const PhysicalDeviceFeatures& features, const bool enabled)
{
        std::vector<std::string> res;

        features_to_strings(features.features_10, enabled, &res);
        features_to_strings(features.features_11, enabled, &res);
        features_to_strings(features.features_12, enabled, &res);
        features_to_strings(features.acceleration_structure, enabled, &res);
        features_to_strings(features.ray_query, enabled, &res);
        features_to_strings(features.ray_tracing_pipeline, enabled, &res);

        return res;
}

template <typename Features>
bool any_feature_enabled(const Features& features)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* ptr = reinterpret_cast<const std::byte*>(&features) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, ptr, SIZE);
                if (feature)
                {
                        return true;
                }
        }

        return false;
}

template bool any_feature_enabled(const VkPhysicalDeviceFeatures&);
template bool any_feature_enabled(const VkPhysicalDeviceVulkan11Features&);
template bool any_feature_enabled(const VkPhysicalDeviceVulkan12Features&);
template bool any_feature_enabled(const VkPhysicalDeviceAccelerationStructureFeaturesKHR&);
template bool any_feature_enabled(const VkPhysicalDeviceRayQueryFeaturesKHR&);
template bool any_feature_enabled(const VkPhysicalDeviceRayTracingPipelineFeaturesKHR&);
}
