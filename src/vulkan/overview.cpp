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

std::vector<unsigned> samples(VkSampleCountFlags flags)
{
        std::vector<unsigned> samples;
        if (flags & VK_SAMPLE_COUNT_1_BIT)
        {
                samples.push_back(1);
        }
        if (flags & VK_SAMPLE_COUNT_2_BIT)
        {
                samples.push_back(2);
        }
        if (flags & VK_SAMPLE_COUNT_4_BIT)
        {
                samples.push_back(4);
        }
        if (flags & VK_SAMPLE_COUNT_8_BIT)
        {
                samples.push_back(8);
        }
        if (flags & VK_SAMPLE_COUNT_16_BIT)
        {
                samples.push_back(16);
        }
        if (flags & VK_SAMPLE_COUNT_32_BIT)
        {
                samples.push_back(32);
        }
        if (flags & VK_SAMPLE_COUNT_64_BIT)
        {
                samples.push_back(64);
        }
        return samples;
}

void device_name(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t type_node = tree->add(device_node, "Device Name");
        tree->add(type_node, device.properties().deviceName);
}

void device_type(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t type_node = tree->add(device_node, "Device Type");
        try
        {
                tree->add(type_node, physical_device_type_to_string(device.properties().deviceType));
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
                tree->add(api_node, api_version_to_string(device.properties().apiVersion));
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

#define ADD_VALUE(v) limits.emplace_back(#v, to_string(value(device.properties().limits.v)))
#define ADD_SAMPLE(v) limits.emplace_back(#v, to_string(samples(device.properties().limits.v)))

void limits(const PhysicalDevice& device, size_t device_node, StringTree* tree)
{
        size_t limits_node = tree->add(device_node, "Limits");

        try
        {
                std::vector<std::tuple<std::string, std::string>> limits;

                ADD_VALUE(maxImageDimension1D);
                ADD_VALUE(maxImageDimension2D);
                ADD_VALUE(maxImageDimension3D);
                ADD_VALUE(maxImageDimensionCube);
                ADD_VALUE(maxImageArrayLayers);
                ADD_VALUE(maxTexelBufferElements);
                ADD_VALUE(maxUniformBufferRange);
                ADD_VALUE(maxStorageBufferRange);
                ADD_VALUE(maxPushConstantsSize);
                ADD_VALUE(maxMemoryAllocationCount);
                ADD_VALUE(maxSamplerAllocationCount);
                ADD_VALUE(bufferImageGranularity);
                ADD_VALUE(sparseAddressSpaceSize);
                ADD_VALUE(maxBoundDescriptorSets);
                ADD_VALUE(maxPerStageDescriptorSamplers);
                ADD_VALUE(maxPerStageDescriptorUniformBuffers);
                ADD_VALUE(maxPerStageDescriptorStorageBuffers);
                ADD_VALUE(maxPerStageDescriptorSampledImages);
                ADD_VALUE(maxPerStageDescriptorStorageImages);
                ADD_VALUE(maxPerStageDescriptorInputAttachments);
                ADD_VALUE(maxPerStageResources);
                ADD_VALUE(maxDescriptorSetSamplers);
                ADD_VALUE(maxDescriptorSetUniformBuffers);
                ADD_VALUE(maxDescriptorSetUniformBuffersDynamic);
                ADD_VALUE(maxDescriptorSetStorageBuffers);
                ADD_VALUE(maxDescriptorSetStorageBuffersDynamic);
                ADD_VALUE(maxDescriptorSetSampledImages);
                ADD_VALUE(maxDescriptorSetStorageImages);
                ADD_VALUE(maxDescriptorSetInputAttachments);
                ADD_VALUE(maxVertexInputAttributes);
                ADD_VALUE(maxVertexInputBindings);
                ADD_VALUE(maxVertexInputAttributeOffset);
                ADD_VALUE(maxVertexInputBindingStride);
                ADD_VALUE(maxVertexOutputComponents);
                ADD_VALUE(maxTessellationGenerationLevel);
                ADD_VALUE(maxTessellationPatchSize);
                ADD_VALUE(maxTessellationControlPerVertexInputComponents);
                ADD_VALUE(maxTessellationControlPerVertexOutputComponents);
                ADD_VALUE(maxTessellationControlPerPatchOutputComponents);
                ADD_VALUE(maxTessellationControlTotalOutputComponents);
                ADD_VALUE(maxTessellationEvaluationInputComponents);
                ADD_VALUE(maxTessellationEvaluationOutputComponents);
                ADD_VALUE(maxGeometryShaderInvocations);
                ADD_VALUE(maxGeometryInputComponents);
                ADD_VALUE(maxGeometryOutputComponents);
                ADD_VALUE(maxGeometryOutputVertices);
                ADD_VALUE(maxGeometryTotalOutputComponents);
                ADD_VALUE(maxFragmentInputComponents);
                ADD_VALUE(maxFragmentOutputAttachments);
                ADD_VALUE(maxFragmentDualSrcAttachments);
                ADD_VALUE(maxFragmentCombinedOutputResources);
                ADD_VALUE(maxComputeSharedMemorySize);
                ADD_VALUE(maxComputeWorkGroupCount);
                ADD_VALUE(maxComputeWorkGroupInvocations);
                ADD_VALUE(maxComputeWorkGroupSize);
                ADD_VALUE(subPixelPrecisionBits);
                ADD_VALUE(subTexelPrecisionBits);
                ADD_VALUE(mipmapPrecisionBits);
                ADD_VALUE(maxDrawIndexedIndexValue);
                ADD_VALUE(maxDrawIndirectCount);
                ADD_VALUE(maxSamplerLodBias);
                ADD_VALUE(maxSamplerAnisotropy);
                ADD_VALUE(maxViewports);
                ADD_VALUE(maxViewportDimensions);
                ADD_VALUE(viewportBoundsRange);
                ADD_VALUE(viewportSubPixelBits);
                ADD_VALUE(minMemoryMapAlignment);
                ADD_VALUE(minTexelBufferOffsetAlignment);
                ADD_VALUE(minUniformBufferOffsetAlignment);
                ADD_VALUE(minStorageBufferOffsetAlignment);
                ADD_VALUE(minTexelOffset);
                ADD_VALUE(maxTexelOffset);
                ADD_VALUE(minTexelGatherOffset);
                ADD_VALUE(maxTexelGatherOffset);
                ADD_VALUE(minInterpolationOffset);
                ADD_VALUE(maxInterpolationOffset);
                ADD_VALUE(subPixelInterpolationOffsetBits);
                ADD_VALUE(maxFramebufferWidth);
                ADD_VALUE(maxFramebufferHeight);
                ADD_VALUE(maxFramebufferLayers);
                ADD_SAMPLE(framebufferColorSampleCounts);
                ADD_SAMPLE(framebufferDepthSampleCounts);
                ADD_SAMPLE(framebufferStencilSampleCounts);
                ADD_SAMPLE(framebufferNoAttachmentsSampleCounts);
                ADD_VALUE(maxColorAttachments);
                ADD_SAMPLE(sampledImageColorSampleCounts);
                ADD_SAMPLE(sampledImageIntegerSampleCounts);
                ADD_SAMPLE(sampledImageDepthSampleCounts);
                ADD_SAMPLE(sampledImageStencilSampleCounts);
                ADD_SAMPLE(storageImageSampleCounts);
                ADD_VALUE(maxSampleMaskWords);
                ADD_VALUE(maxClipDistances);
                ADD_VALUE(maxCullDistances);
                ADD_VALUE(maxCombinedClipAndCullDistances);
                ADD_VALUE(discreteQueuePriorities);
                ADD_VALUE(pointSizeRange);
                ADD_VALUE(lineWidthRange);
                ADD_VALUE(pointSizeGranularity);
                ADD_VALUE(lineWidthGranularity);
                ADD_VALUE(strictLines);
                ADD_VALUE(standardSampleLocations);
                ADD_VALUE(optimalBufferCopyOffsetAlignment);
                ADD_VALUE(optimalBufferCopyRowPitchAlignment);
                ADD_VALUE(nonCoherentAtomSize);

                std::sort(limits.begin(), limits.end(), [](const auto& v1, const auto& v2) {
                        return std::get<0>(v1) < std::get<0>(v2);
                });

                for (const auto& [name, value] : limits)
                {
                        tree->add(limits_node, name + " = " + value);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(limits_node, e.what());
        }
}

void queues(
        const PhysicalDevice& device,
        const std::vector<VkQueueFamilyProperties>& families,
        size_t family_index,
        size_t queue_families_node,
        StringTree* tree)
{
        const VkQueueFamilyProperties& p = families[family_index];

        size_t queue_family_node = tree->add(queue_families_node, "Family " + to_string(family_index));

        try
        {
                tree->add(queue_family_node, "queue count: " + to_string(p.queueCount));

                if (p.queueCount < 1)
                {
                        return;
                }

                if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                        tree->add(queue_family_node, "graphics");
                }
                if (p.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                        tree->add(queue_family_node, "compute");
                }
                if (p.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                        tree->add(queue_family_node, "transfer");
                }
                if (p.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                        tree->add(queue_family_node, "sparse binding");
                }
                if (p.queueFlags & VK_QUEUE_PROTECTED_BIT)
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
                std::vector<VkQueueFamilyProperties> families = device.queue_families();

                for (size_t family_index = 0; family_index < families.size(); ++family_index)
                {
                        queues(device, families, family_index, queue_families_node, tree);
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

                if (!uuids.emplace(to_string(value(device.properties().pipelineCacheUUID))).second)
                {
                        continue;
                }

                size_t node = tree.add("Physical Device");

                device_name(device, node, &tree);
                device_type(device, node, &tree);
                api_version(device, node, &tree);
                extensions(device, node, &tree);
                limits(device, node, &tree);
                queue_families(device, node, &tree);
        }

        return tree.text(TREE_LEVEL_INDENT);
}
}
