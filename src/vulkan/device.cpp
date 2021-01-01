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

#include "device.h"

#include "error.h"
#include "overview.h"
#include "query.h"
#include "surface.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/vector.h>

#include <algorithm>

namespace ns::vulkan
{
namespace
{
bool find_family(
        const std::vector<VkQueueFamilyProperties>& families,
        VkQueueFlags flags,
        VkQueueFlags no_flags,
        uint32_t* index)
{
        ASSERT(flags != 0);
        ASSERT((flags & no_flags) == 0);

        for (std::size_t i = 0; i < families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = families[i];

                if (p.queueCount < 1)
                {
                        continue;
                }

                if (((p.queueFlags & flags) == flags) && !(p.queueFlags & no_flags))
                {
                        *index = i;
                        return true;
                }
        }
        return false;
}

std::vector<bool> find_presentation_support(
        VkSurfaceKHR surface,
        VkPhysicalDevice device,
        const std::vector<VkQueueFamilyProperties>& queue_families)
{
        if (surface == VK_NULL_HANDLE)
        {
                return std::vector<bool>(queue_families.size(), false);
        }

        std::vector<bool> presentation_supported(queue_families.size());

        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                if (queue_families[i].queueCount < 1)
                {
                        continue;
                }

                VkBool32 supported;

                VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
                if (result != VK_SUCCESS)
                {
                        vulkan_function_error("vkGetPhysicalDeviceSurfaceSupportKHR", result);
                }

                presentation_supported[i] = (supported == VK_TRUE);
        }

        return presentation_supported;
}

class FeatureIsNotSupported final : public std::exception
{
        const char* m_text;

public:
        explicit FeatureIsNotSupported(const char* feature_name) noexcept : m_text(feature_name)
        {
        }
        const char* what() const noexcept override
        {
                return m_text;
        }
};

[[noreturn]] void feature_is_not_supported(const char* feature_name)
{
        throw FeatureIsNotSupported(feature_name);
}

#define CASE_FEATURE_10(feature)                                                                           \
        case PhysicalDeviceFeatures::feature:                                                              \
                if (!device_features.features_10.feature && required)                                      \
                {                                                                                          \
                        feature_is_not_supported(#feature);                                                \
                }                                                                                          \
                if (result_device_features)                                                                \
                {                                                                                          \
                        result_device_features->features_10.feature = device_features.features_10.feature; \
                }                                                                                          \
                break;

#define CASE_FEATURE_11(feature)                                                                           \
        case PhysicalDeviceFeatures::feature:                                                              \
                if (!device_features.features_11.feature && required)                                      \
                {                                                                                          \
                        feature_is_not_supported(#feature);                                                \
                }                                                                                          \
                if (result_device_features)                                                                \
                {                                                                                          \
                        result_device_features->features_11.feature = device_features.features_11.feature; \
                }                                                                                          \
                break;

#define CASE_FEATURE_12(feature)                                                                           \
        case PhysicalDeviceFeatures::feature:                                                              \
                if (!device_features.features_12.feature && required)                                      \
                {                                                                                          \
                        feature_is_not_supported(#feature);                                                \
                }                                                                                          \
                if (result_device_features)                                                                \
                {                                                                                          \
                        result_device_features->features_12.feature = device_features.features_12.feature; \
                }                                                                                          \
                break;

void set_features(
        const std::vector<PhysicalDeviceFeatures>& features,
        bool required,
        const DeviceFeatures& device_features,
        DeviceFeatures* result_device_features = nullptr)
{
        for (PhysicalDeviceFeatures f : features)
        {
                switch (f)
                {
                        CASE_FEATURE_10(alphaToOne)
                        CASE_FEATURE_10(depthBiasClamp)
                        CASE_FEATURE_10(depthBounds)
                        CASE_FEATURE_10(depthClamp)
                        CASE_FEATURE_10(drawIndirectFirstInstance)
                        CASE_FEATURE_10(dualSrcBlend)
                        CASE_FEATURE_10(fillModeNonSolid)
                        CASE_FEATURE_10(fragmentStoresAndAtomics)
                        CASE_FEATURE_10(fullDrawIndexUint32)
                        CASE_FEATURE_10(geometryShader)
                        CASE_FEATURE_10(imageCubeArray)
                        CASE_FEATURE_10(independentBlend)
                        CASE_FEATURE_10(inheritedQueries)
                        CASE_FEATURE_10(largePoints)
                        CASE_FEATURE_10(logicOp)
                        CASE_FEATURE_10(multiDrawIndirect)
                        CASE_FEATURE_10(multiViewport)
                        CASE_FEATURE_10(occlusionQueryPrecise)
                        CASE_FEATURE_10(pipelineStatisticsQuery)
                        CASE_FEATURE_10(robustBufferAccess)
                        CASE_FEATURE_10(sampleRateShading)
                        CASE_FEATURE_10(samplerAnisotropy)
                        CASE_FEATURE_10(shaderClipDistance)
                        CASE_FEATURE_10(shaderCullDistance)
                        CASE_FEATURE_10(shaderFloat64)
                        CASE_FEATURE_10(shaderImageGatherExtended)
                        CASE_FEATURE_10(shaderInt16)
                        CASE_FEATURE_10(shaderInt64)
                        CASE_FEATURE_10(shaderResourceMinLod)
                        CASE_FEATURE_10(shaderResourceResidency)
                        CASE_FEATURE_10(shaderSampledImageArrayDynamicIndexing)
                        CASE_FEATURE_10(shaderStorageBufferArrayDynamicIndexing)
                        CASE_FEATURE_10(shaderStorageImageArrayDynamicIndexing)
                        CASE_FEATURE_10(shaderStorageImageExtendedFormats)
                        CASE_FEATURE_10(shaderStorageImageMultisample)
                        CASE_FEATURE_10(shaderStorageImageReadWithoutFormat)
                        CASE_FEATURE_10(shaderStorageImageWriteWithoutFormat)
                        CASE_FEATURE_10(shaderTessellationAndGeometryPointSize)
                        CASE_FEATURE_10(shaderUniformBufferArrayDynamicIndexing)
                        CASE_FEATURE_10(sparseBinding)
                        CASE_FEATURE_10(sparseResidency16Samples)
                        CASE_FEATURE_10(sparseResidency2Samples)
                        CASE_FEATURE_10(sparseResidency4Samples)
                        CASE_FEATURE_10(sparseResidency8Samples)
                        CASE_FEATURE_10(sparseResidencyAliased)
                        CASE_FEATURE_10(sparseResidencyBuffer)
                        CASE_FEATURE_10(sparseResidencyImage2D)
                        CASE_FEATURE_10(sparseResidencyImage3D)
                        CASE_FEATURE_10(tessellationShader)
                        CASE_FEATURE_10(textureCompressionASTC_LDR)
                        CASE_FEATURE_10(textureCompressionBC)
                        CASE_FEATURE_10(textureCompressionETC2)
                        CASE_FEATURE_10(variableMultisampleRate)
                        CASE_FEATURE_10(vertexPipelineStoresAndAtomics)
                        CASE_FEATURE_10(wideLines)
                        //
                        CASE_FEATURE_11(multiview)
                        CASE_FEATURE_11(multiviewGeometryShader)
                        CASE_FEATURE_11(multiviewTessellationShader)
                        CASE_FEATURE_11(protectedMemory)
                        CASE_FEATURE_11(samplerYcbcrConversion)
                        CASE_FEATURE_11(shaderDrawParameters)
                        CASE_FEATURE_11(storageBuffer16BitAccess)
                        CASE_FEATURE_11(storageInputOutput16)
                        CASE_FEATURE_11(storagePushConstant16)
                        CASE_FEATURE_11(uniformAndStorageBuffer16BitAccess)
                        CASE_FEATURE_11(variablePointers)
                        CASE_FEATURE_11(variablePointersStorageBuffer)
                        //
                        CASE_FEATURE_12(bufferDeviceAddress)
                        CASE_FEATURE_12(bufferDeviceAddressCaptureReplay)
                        CASE_FEATURE_12(bufferDeviceAddressMultiDevice)
                        CASE_FEATURE_12(descriptorBindingPartiallyBound)
                        CASE_FEATURE_12(descriptorBindingSampledImageUpdateAfterBind)
                        CASE_FEATURE_12(descriptorBindingStorageBufferUpdateAfterBind)
                        CASE_FEATURE_12(descriptorBindingStorageImageUpdateAfterBind)
                        CASE_FEATURE_12(descriptorBindingStorageTexelBufferUpdateAfterBind)
                        CASE_FEATURE_12(descriptorBindingUniformBufferUpdateAfterBind)
                        CASE_FEATURE_12(descriptorBindingUniformTexelBufferUpdateAfterBind)
                        CASE_FEATURE_12(descriptorBindingUpdateUnusedWhilePending)
                        CASE_FEATURE_12(descriptorBindingVariableDescriptorCount)
                        CASE_FEATURE_12(descriptorIndexing)
                        CASE_FEATURE_12(drawIndirectCount)
                        CASE_FEATURE_12(hostQueryReset)
                        CASE_FEATURE_12(imagelessFramebuffer)
                        CASE_FEATURE_12(runtimeDescriptorArray)
                        CASE_FEATURE_12(samplerFilterMinmax)
                        CASE_FEATURE_12(samplerMirrorClampToEdge)
                        CASE_FEATURE_12(scalarBlockLayout)
                        CASE_FEATURE_12(separateDepthStencilLayouts)
                        CASE_FEATURE_12(shaderBufferInt64Atomics)
                        CASE_FEATURE_12(shaderFloat16)
                        CASE_FEATURE_12(shaderInputAttachmentArrayDynamicIndexing)
                        CASE_FEATURE_12(shaderInputAttachmentArrayNonUniformIndexing)
                        CASE_FEATURE_12(shaderInt8)
                        CASE_FEATURE_12(shaderOutputLayer)
                        CASE_FEATURE_12(shaderOutputViewportIndex)
                        CASE_FEATURE_12(shaderSampledImageArrayNonUniformIndexing)
                        CASE_FEATURE_12(shaderSharedInt64Atomics)
                        CASE_FEATURE_12(shaderStorageBufferArrayNonUniformIndexing)
                        CASE_FEATURE_12(shaderStorageImageArrayNonUniformIndexing)
                        CASE_FEATURE_12(shaderStorageTexelBufferArrayDynamicIndexing)
                        CASE_FEATURE_12(shaderStorageTexelBufferArrayNonUniformIndexing)
                        CASE_FEATURE_12(shaderSubgroupExtendedTypes)
                        CASE_FEATURE_12(shaderUniformBufferArrayNonUniformIndexing)
                        CASE_FEATURE_12(shaderUniformTexelBufferArrayDynamicIndexing)
                        CASE_FEATURE_12(shaderUniformTexelBufferArrayNonUniformIndexing)
                        CASE_FEATURE_12(storageBuffer8BitAccess)
                        CASE_FEATURE_12(storagePushConstant8)
                        CASE_FEATURE_12(subgroupBroadcastDynamicId)
                        CASE_FEATURE_12(timelineSemaphore)
                        CASE_FEATURE_12(uniformAndStorageBuffer8BitAccess)
                        CASE_FEATURE_12(uniformBufferStandardLayout)
                        CASE_FEATURE_12(vulkanMemoryModel)
                        CASE_FEATURE_12(vulkanMemoryModelAvailabilityVisibilityChains)
                        CASE_FEATURE_12(vulkanMemoryModelDeviceScope)
                }
        }
}

std::vector<VkQueueFamilyProperties> find_queue_families(VkPhysicalDevice device)
{
        uint32_t queue_family_count;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        if (queue_family_count < 1)
        {
                return {};
        }

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        return queue_families;
}

std::unordered_set<std::string> find_extensions(VkPhysicalDevice device)
{
        uint32_t extension_count;
        VkResult result;

        result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        std::unordered_set<std::string> extension_set;

        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }

        return extension_set;
}

void make_enabled_device_features(
        const std::vector<PhysicalDeviceFeatures>& required_features,
        const std::vector<PhysicalDeviceFeatures>& optional_features,
        const DeviceFeatures& supported_device_features,
        DeviceFeatures* device_features)
{
        if (there_is_intersection(required_features, optional_features))
        {
                error("Required and optional physical device features intersect");
        }

        *device_features = {};

        try
        {
                set_features(required_features, true, supported_device_features, device_features);
        }
        catch (const FeatureIsNotSupported& e)
        {
                error(std::string("Required physical device feature ") + e.what() + " is not supported");
        }

        try
        {
                set_features(optional_features, false, supported_device_features, device_features);
        }
        catch (const FeatureIsNotSupported&)
        {
                error("Exception when setting optional device features");
        }
}
}

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
        : m_physical_device(physical_device)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        {
                VkPhysicalDeviceVulkan12Properties vulkan_12_properties = {};
                vulkan_12_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
                VkPhysicalDeviceVulkan11Properties vulkan_11_properties = {};
                vulkan_11_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
                vulkan_11_properties.pNext = &vulkan_12_properties;
                VkPhysicalDeviceProperties2 properties_2 = {};
                properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                properties_2.pNext = &vulkan_11_properties;
                vkGetPhysicalDeviceProperties2(m_physical_device, &properties_2);

                m_properties.properties_10 = properties_2.properties;
                m_properties.properties_11 = vulkan_11_properties;
                m_properties.properties_11.pNext = nullptr;
                m_properties.properties_12 = vulkan_12_properties;
                m_properties.properties_12.pNext = nullptr;
        }
        {
                VkPhysicalDeviceVulkan12Features vulkan_12_features = {};
                vulkan_12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                VkPhysicalDeviceVulkan11Features vulkan_11_features = {};
                vulkan_11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
                vulkan_11_features.pNext = &vulkan_12_features;
                VkPhysicalDeviceFeatures2 features_2 = {};
                features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                features_2.pNext = &vulkan_11_features;
                vkGetPhysicalDeviceFeatures2(m_physical_device, &features_2);

                m_features.features_10 = features_2.features;
                m_features.features_11 = vulkan_11_features;
                m_features.features_11.pNext = nullptr;
                m_features.features_12 = vulkan_12_features;
                m_features.features_12.pNext = nullptr;
        }

        m_queue_families = find_queue_families(physical_device);
        m_presentation_supported = find_presentation_support(surface, m_physical_device, m_queue_families);
        m_supported_extensions = find_extensions(m_physical_device);

        ASSERT(m_queue_families.size() == m_presentation_supported.size());
}

PhysicalDevice::operator VkPhysicalDevice() const&
{
        return m_physical_device;
}

const DeviceFeatures& PhysicalDevice::features() const
{
        return m_features;
}

const DeviceProperties& PhysicalDevice::properties() const
{
        return m_properties;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::queue_families() const
{
        return m_queue_families;
}

const std::unordered_set<std::string>& PhysicalDevice::supported_extensions() const
{
        return m_supported_extensions;
}

uint32_t PhysicalDevice::family_index(VkQueueFlags set_flags, VkQueueFlags not_set_flags, VkQueueFlags default_flags)
        const
{
        uint32_t index;
        if (set_flags && find_family(m_queue_families, set_flags, not_set_flags, &index))
        {
                return index;
        }
        if (default_flags && find_family(m_queue_families, default_flags, 0, &index))
        {
                return index;
        }
        error("Queue family not found, flags " + to_string(set_flags) + " " + to_string(not_set_flags) + " "
              + to_string(default_flags));
}

uint32_t PhysicalDevice::presentation_family_index() const
{
        for (std::size_t i = 0; i < m_presentation_supported.size(); ++i)
        {
                if (m_presentation_supported[i])
                {
                        return i;
                }
        }
        error("Presentation family not found");
}

bool PhysicalDevice::supports_extensions(const std::vector<std::string>& extensions) const
{
        return std::all_of(
                extensions.cbegin(), extensions.cend(),
                [&](const std::string& e)
                {
                        return m_supported_extensions.count(e) >= 1;
                });
}

bool PhysicalDevice::queue_family_supports_presentation(uint32_t index) const
{
        ASSERT(index < m_presentation_supported.size());

        return m_presentation_supported[index];
}

//

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance)
{
        uint32_t device_count;
        VkResult result;

        result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        if (device_count < 1)
        {
                error("No Vulkan device found");
        }

        std::vector<VkPhysicalDevice> devices(device_count);

        result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        return devices;
}

PhysicalDevice create_physical_device(
        VkInstance instance,
        VkSurfaceKHR surface,
        int api_version_major,
        int api_version_minor,
        const std::vector<std::string>& required_extensions,
        const std::vector<PhysicalDeviceFeatures>& required_features)
{
        LOG(overview_physical_devices(instance, surface));

        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& d : physical_devices(instance))
        {
                PhysicalDevice physical_device(d, surface);

                if (physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                    && physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
                    && physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU
                    && physical_device.properties().properties_10.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (required_api_version > physical_device.properties().properties_10.apiVersion)
                {
                        continue;
                }

                try
                {
                        set_features(required_features, true, physical_device.features());
                }
                catch (const FeatureIsNotSupported&)
                {
                        continue;
                }

                if (!physical_device.supports_extensions(required_extensions))
                {
                        continue;
                }

                try
                {
                        physical_device.family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, 0);
                }
                catch (...)
                {
                        continue;
                }

                if (surface != VK_NULL_HANDLE)
                {
                        try
                        {
                                physical_device.presentation_family_index();
                        }
                        catch (...)
                        {
                                continue;
                        }

                        if (!surface_suitable(surface, physical_device))
                        {
                                continue;
                        }
                }

                return physical_device;
        }

        error("Failed to find a suitable Vulkan physical device");
}

Device create_device(
        const PhysicalDevice& physical_device,
        const std::unordered_map<uint32_t, uint32_t>& queue_families,
        const std::vector<std::string>& required_extensions,
        const std::vector<PhysicalDeviceFeatures>& required_features,
        const std::vector<PhysicalDeviceFeatures>& optional_features)
{
        ASSERT(std::all_of(
                queue_families.cbegin(), queue_families.cend(),
                [&](const auto& v)
                {
                        return v.first < physical_device.queue_families().size();
                }));
        ASSERT(std::all_of(
                queue_families.cbegin(), queue_families.cend(),
                [](const auto& v)
                {
                        return v.second > 0;
                }));
        ASSERT(std::all_of(
                queue_families.cbegin(), queue_families.cend(),
                [&](const auto& v)
                {
                        return v.second <= physical_device.queue_families()[v.first].queueCount;
                }));

        if (queue_families.empty())
        {
                error("No queue families for device creation");
        }

        std::vector<std::vector<float>> queue_priorities(queue_families.size());
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_families.size());
        unsigned i = 0;
        for (const auto& [queue_family_index, queue_count] : queue_families)
        {
                queue_priorities[i].resize(queue_count, 1);

                VkDeviceQueueCreateInfo queue_create_info = {};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = queue_family_index;
                queue_create_info.queueCount = queue_count;
                queue_create_info.pQueuePriorities = queue_priorities[i].data();
                queue_create_infos[i] = queue_create_info;

                ++i;
        }

        DeviceFeatures enabled_features = {};
        make_enabled_device_features(
                required_features, optional_features, physical_device.features(), &enabled_features);

        enabled_features.features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        enabled_features.features_12.pNext = nullptr;
        enabled_features.features_11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        enabled_features.features_11.pNext = &enabled_features.features_12;
        VkPhysicalDeviceFeatures2 features_2 = {};
        features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features_2.pNext = &enabled_features.features_11;
        features_2.features = enabled_features.features_10;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pNext = &features_2;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (!extensions.empty())
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        return Device(physical_device, &physical_device.properties(), create_info);
}
}
