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

#include "device.h"

#include "error.h"
#include "overview.h"
#include "query.h"
#include "surface.h"

#include "com/alg.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/string/vector.h"

#include <algorithm>

namespace vulkan
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

        for (size_t i = 0; i < families.size(); ++i)
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

void set_features(
        const std::vector<PhysicalDeviceFeatures>& features,
        bool required,
        const VkPhysicalDeviceFeatures& device_features,
        VkPhysicalDeviceFeatures* result_device_features = nullptr)
{
        for (PhysicalDeviceFeatures f : features)
        {
                switch (f)
                {
                case PhysicalDeviceFeatures::alphaToOne:
                        if (!device_features.alphaToOne && required)
                        {
                                feature_is_not_supported("alphaToOne");
                        }
                        if (result_device_features)
                        {
                                result_device_features->alphaToOne = device_features.alphaToOne;
                        }
                        break;
                case PhysicalDeviceFeatures::depthBiasClamp:
                        if (!device_features.depthBiasClamp && required)
                        {
                                feature_is_not_supported("depthBiasClamp");
                        }
                        if (result_device_features)
                        {
                                result_device_features->depthBiasClamp = device_features.depthBiasClamp;
                        }
                        break;
                case PhysicalDeviceFeatures::depthBounds:
                        if (!device_features.depthBounds && required)
                        {
                                feature_is_not_supported("depthBounds");
                        }
                        if (result_device_features)
                        {
                                result_device_features->depthBounds = device_features.depthBounds;
                        }
                        break;
                case PhysicalDeviceFeatures::depthClamp:
                        if (!device_features.depthClamp && required)
                        {
                                feature_is_not_supported("depthClamp");
                        }
                        if (result_device_features)
                        {
                                result_device_features->depthClamp = device_features.depthClamp;
                        }
                        break;
                case PhysicalDeviceFeatures::drawIndirectFirstInstance:
                        if (!device_features.drawIndirectFirstInstance && required)
                        {
                                feature_is_not_supported("drawIndirectFirstInstance");
                        }
                        if (result_device_features)
                        {
                                result_device_features->drawIndirectFirstInstance =
                                        device_features.drawIndirectFirstInstance;
                        }
                        break;
                case PhysicalDeviceFeatures::dualSrcBlend:
                        if (!device_features.dualSrcBlend && required)
                        {
                                feature_is_not_supported("dualSrcBlend");
                        }
                        if (result_device_features)
                        {
                                result_device_features->dualSrcBlend = device_features.dualSrcBlend;
                        }
                        break;
                case PhysicalDeviceFeatures::fillModeNonSolid:
                        if (!device_features.fillModeNonSolid && required)
                        {
                                feature_is_not_supported("fillModeNonSolid");
                        }
                        if (result_device_features)
                        {
                                result_device_features->fillModeNonSolid = device_features.fillModeNonSolid;
                        }
                        break;
                case PhysicalDeviceFeatures::fragmentStoresAndAtomics:
                        if (!device_features.fragmentStoresAndAtomics && required)
                        {
                                feature_is_not_supported("fragmentStoresAndAtomics");
                        }
                        if (result_device_features)
                        {
                                result_device_features->fragmentStoresAndAtomics =
                                        device_features.fragmentStoresAndAtomics;
                        }
                        break;
                case PhysicalDeviceFeatures::fullDrawIndexUint32:
                        if (!device_features.fullDrawIndexUint32 && required)
                        {
                                feature_is_not_supported("fullDrawIndexUint32");
                        }
                        if (result_device_features)
                        {
                                result_device_features->fullDrawIndexUint32 = device_features.fullDrawIndexUint32;
                        }
                        break;
                case PhysicalDeviceFeatures::geometryShader:
                        if (!device_features.geometryShader && required)
                        {
                                feature_is_not_supported("geometryShader");
                        }
                        if (result_device_features)
                        {
                                result_device_features->geometryShader = device_features.geometryShader;
                        }
                        break;
                case PhysicalDeviceFeatures::imageCubeArray:
                        if (!device_features.imageCubeArray && required)
                        {
                                feature_is_not_supported("imageCubeArray");
                        }
                        if (result_device_features)
                        {
                                result_device_features->imageCubeArray = device_features.imageCubeArray;
                        }
                        break;
                case PhysicalDeviceFeatures::independentBlend:
                        if (!device_features.independentBlend && required)
                        {
                                feature_is_not_supported("independentBlend");
                        }
                        if (result_device_features)
                        {
                                result_device_features->independentBlend = device_features.independentBlend;
                        }
                        break;
                case PhysicalDeviceFeatures::inheritedQueries:
                        if (!device_features.inheritedQueries && required)
                        {
                                feature_is_not_supported("inheritedQueries");
                        }
                        if (result_device_features)
                        {
                                result_device_features->inheritedQueries = device_features.inheritedQueries;
                        }
                        break;
                case PhysicalDeviceFeatures::largePoints:
                        if (!device_features.largePoints && required)
                        {
                                feature_is_not_supported("largePoints");
                        }
                        if (result_device_features)
                        {
                                result_device_features->largePoints = device_features.largePoints;
                        }
                        break;
                case PhysicalDeviceFeatures::logicOp:
                        if (!device_features.logicOp && required)
                        {
                                feature_is_not_supported("logicOp");
                        }
                        if (result_device_features)
                        {
                                result_device_features->logicOp = device_features.logicOp;
                        }
                        break;
                case PhysicalDeviceFeatures::multiDrawIndirect:
                        if (!device_features.multiDrawIndirect && required)
                        {
                                feature_is_not_supported("multiDrawIndirect");
                        }
                        if (result_device_features)
                        {
                                result_device_features->multiDrawIndirect = device_features.multiDrawIndirect;
                        }
                        break;
                case PhysicalDeviceFeatures::multiViewport:
                        if (!device_features.multiViewport && required)
                        {
                                feature_is_not_supported("multiViewport");
                        }
                        if (result_device_features)
                        {
                                result_device_features->multiViewport = device_features.multiViewport;
                        }
                        break;
                case PhysicalDeviceFeatures::occlusionQueryPrecise:
                        if (!device_features.occlusionQueryPrecise && required)
                        {
                                feature_is_not_supported("occlusionQueryPrecise");
                        }
                        if (result_device_features)
                        {
                                result_device_features->occlusionQueryPrecise = device_features.occlusionQueryPrecise;
                        }
                        break;
                case PhysicalDeviceFeatures::pipelineStatisticsQuery:
                        if (!device_features.pipelineStatisticsQuery && required)
                        {
                                feature_is_not_supported("pipelineStatisticsQuery");
                        }
                        if (result_device_features)
                        {
                                result_device_features->pipelineStatisticsQuery =
                                        device_features.pipelineStatisticsQuery;
                        }
                        break;
                case PhysicalDeviceFeatures::robustBufferAccess:
                        if (!device_features.robustBufferAccess && required)
                        {
                                feature_is_not_supported("robustBufferAccess");
                        }
                        if (result_device_features)
                        {
                                result_device_features->robustBufferAccess = device_features.robustBufferAccess;
                        }
                        break;
                case PhysicalDeviceFeatures::sampleRateShading:
                        if (!device_features.sampleRateShading && required)
                        {
                                feature_is_not_supported("sampleRateShading");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sampleRateShading = device_features.sampleRateShading;
                        }
                        break;
                case PhysicalDeviceFeatures::samplerAnisotropy:
                        if (!device_features.samplerAnisotropy && required)
                        {
                                feature_is_not_supported("samplerAnisotropy");
                        }
                        if (result_device_features)
                        {
                                result_device_features->samplerAnisotropy = device_features.samplerAnisotropy;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderClipDistance:
                        if (!device_features.shaderClipDistance && required)
                        {
                                feature_is_not_supported("shaderClipDistance");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderClipDistance = device_features.shaderClipDistance;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderCullDistance:
                        if (!device_features.shaderCullDistance && required)
                        {
                                feature_is_not_supported("shaderCullDistance");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderCullDistance = device_features.shaderCullDistance;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderFloat64:
                        if (!device_features.shaderFloat64 && required)
                        {
                                feature_is_not_supported("shaderFloat64");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderFloat64 = device_features.shaderFloat64;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderImageGatherExtended:
                        if (!device_features.shaderImageGatherExtended && required)
                        {
                                feature_is_not_supported("shaderImageGatherExtended");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderImageGatherExtended =
                                        device_features.shaderImageGatherExtended;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderInt16:
                        if (!device_features.shaderInt16 && required)
                        {
                                feature_is_not_supported("shaderInt16");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderInt16 = device_features.shaderInt16;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderInt64:
                        if (!device_features.shaderInt64 && required)
                        {
                                feature_is_not_supported("shaderInt64");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderInt64 = device_features.shaderInt64;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderResourceMinLod:
                        if (!device_features.shaderResourceMinLod && required)
                        {
                                feature_is_not_supported("shaderResourceMinLod");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderResourceMinLod = device_features.shaderResourceMinLod;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderResourceResidency:
                        if (!device_features.shaderResourceResidency && required)
                        {
                                feature_is_not_supported("shaderResourceResidency");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderResourceResidency =
                                        device_features.shaderResourceResidency;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderSampledImageArrayDynamicIndexing:
                        if (!device_features.shaderSampledImageArrayDynamicIndexing && required)
                        {
                                feature_is_not_supported("shaderSampledImageArrayDynamicIndexing");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderSampledImageArrayDynamicIndexing =
                                        device_features.shaderSampledImageArrayDynamicIndexing;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderStorageBufferArrayDynamicIndexing:
                        if (!device_features.shaderStorageBufferArrayDynamicIndexing && required)
                        {
                                feature_is_not_supported("shaderStorageBufferArrayDynamicIndexing");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderStorageBufferArrayDynamicIndexing =
                                        device_features.shaderStorageBufferArrayDynamicIndexing;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderStorageImageArrayDynamicIndexing:
                        if (!device_features.shaderStorageImageArrayDynamicIndexing && required)
                        {
                                feature_is_not_supported("shaderStorageImageArrayDynamicIndexing");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderStorageImageArrayDynamicIndexing =
                                        device_features.shaderStorageImageArrayDynamicIndexing;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderStorageImageExtendedFormats:
                        if (!device_features.shaderStorageImageExtendedFormats && required)
                        {
                                feature_is_not_supported("shaderStorageImageExtendedFormats");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderStorageImageExtendedFormats =
                                        device_features.shaderStorageImageExtendedFormats;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderStorageImageMultisample:
                        if (!device_features.shaderStorageImageMultisample && required)
                        {
                                feature_is_not_supported("shaderStorageImageMultisample");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderStorageImageMultisample =
                                        device_features.shaderStorageImageMultisample;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderStorageImageReadWithoutFormat:
                        if (!device_features.shaderStorageImageReadWithoutFormat && required)
                        {
                                feature_is_not_supported("shaderStorageImageReadWithoutFormat");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderStorageImageReadWithoutFormat =
                                        device_features.shaderStorageImageReadWithoutFormat;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderStorageImageWriteWithoutFormat:
                        if (!device_features.shaderStorageImageWriteWithoutFormat && required)
                        {
                                feature_is_not_supported("shaderStorageImageWriteWithoutFormat");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderStorageImageWriteWithoutFormat =
                                        device_features.shaderStorageImageWriteWithoutFormat;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderTessellationAndGeometryPointSize:
                        if (!device_features.shaderTessellationAndGeometryPointSize && required)
                        {
                                feature_is_not_supported("shaderTessellationAndGeometryPointSize");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderTessellationAndGeometryPointSize =
                                        device_features.shaderTessellationAndGeometryPointSize;
                        }
                        break;
                case PhysicalDeviceFeatures::shaderUniformBufferArrayDynamicIndexing:
                        if (!device_features.shaderUniformBufferArrayDynamicIndexing && required)
                        {
                                feature_is_not_supported("shaderUniformBufferArrayDynamicIndexing");
                        }
                        if (result_device_features)
                        {
                                result_device_features->shaderUniformBufferArrayDynamicIndexing =
                                        device_features.shaderUniformBufferArrayDynamicIndexing;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseBinding:
                        if (!device_features.sparseBinding && required)
                        {
                                feature_is_not_supported("sparseBinding");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseBinding = device_features.sparseBinding;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidency16Samples:
                        if (!device_features.sparseResidency16Samples && required)
                        {
                                feature_is_not_supported("sparseResidency16Samples");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidency16Samples =
                                        device_features.sparseResidency16Samples;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidency2Samples:
                        if (!device_features.sparseResidency2Samples && required)
                        {
                                feature_is_not_supported("sparseResidency2Samples");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidency2Samples =
                                        device_features.sparseResidency2Samples;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidency4Samples:
                        if (!device_features.sparseResidency4Samples && required)
                        {
                                feature_is_not_supported("sparseResidency4Samples");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidency4Samples =
                                        device_features.sparseResidency4Samples;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidency8Samples:
                        if (!device_features.sparseResidency8Samples && required)
                        {
                                feature_is_not_supported("sparseResidency8Samples");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidency8Samples =
                                        device_features.sparseResidency8Samples;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidencyAliased:
                        if (!device_features.sparseResidencyAliased && required)
                        {
                                feature_is_not_supported("sparseResidencyAliased");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidencyAliased = device_features.sparseResidencyAliased;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidencyBuffer:
                        if (!device_features.sparseResidencyBuffer && required)
                        {
                                feature_is_not_supported("sparseResidencyBuffer");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidencyBuffer = device_features.sparseResidencyBuffer;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidencyImage2D:
                        if (!device_features.sparseResidencyImage2D && required)
                        {
                                feature_is_not_supported("sparseResidencyImage2D");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidencyImage2D = device_features.sparseResidencyImage2D;
                        }
                        break;
                case PhysicalDeviceFeatures::sparseResidencyImage3D:
                        if (!device_features.sparseResidencyImage3D && required)
                        {
                                feature_is_not_supported("sparseResidencyImage3D");
                        }
                        if (result_device_features)
                        {
                                result_device_features->sparseResidencyImage3D = device_features.sparseResidencyImage3D;
                        }
                        break;
                case PhysicalDeviceFeatures::tessellationShader:
                        if (!device_features.tessellationShader && required)
                        {
                                feature_is_not_supported("tessellationShader");
                        }
                        if (result_device_features)
                        {
                                result_device_features->tessellationShader = device_features.tessellationShader;
                        }
                        break;
                case PhysicalDeviceFeatures::textureCompressionASTC_LDR:
                        if (!device_features.textureCompressionASTC_LDR && required)
                        {
                                feature_is_not_supported("textureCompressionASTC_LDR");
                        }
                        if (result_device_features)
                        {
                                result_device_features->textureCompressionASTC_LDR =
                                        device_features.textureCompressionASTC_LDR;
                        }
                        break;
                case PhysicalDeviceFeatures::textureCompressionBC:
                        if (!device_features.textureCompressionBC && required)
                        {
                                feature_is_not_supported("textureCompressionBC");
                        }
                        if (result_device_features)
                        {
                                result_device_features->textureCompressionBC = device_features.textureCompressionBC;
                        }
                        break;
                case PhysicalDeviceFeatures::textureCompressionETC2:
                        if (!device_features.textureCompressionETC2 && required)
                        {
                                feature_is_not_supported("textureCompressionETC2");
                        }
                        if (result_device_features)
                        {
                                result_device_features->textureCompressionETC2 = device_features.textureCompressionETC2;
                        }
                        break;
                case PhysicalDeviceFeatures::variableMultisampleRate:
                        if (!device_features.variableMultisampleRate && required)
                        {
                                feature_is_not_supported("variableMultisampleRate");
                        }
                        if (result_device_features)
                        {
                                result_device_features->variableMultisampleRate =
                                        device_features.variableMultisampleRate;
                        }
                        break;
                case PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics:
                        if (!device_features.vertexPipelineStoresAndAtomics && required)
                        {
                                feature_is_not_supported("vertexPipelineStoresAndAtomics");
                        }
                        if (result_device_features)
                        {
                                result_device_features->vertexPipelineStoresAndAtomics =
                                        device_features.vertexPipelineStoresAndAtomics;
                        }
                        break;
                case PhysicalDeviceFeatures::wideLines:
                        if (!device_features.wideLines && required)
                        {
                                feature_is_not_supported("wideLines");
                        }
                        if (result_device_features)
                        {
                                result_device_features->wideLines = device_features.wideLines;
                        }
                        break;
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

VkPhysicalDeviceFeatures make_enabled_device_features(
        const std::vector<PhysicalDeviceFeatures>& required_features,
        const std::vector<PhysicalDeviceFeatures>& optional_features,
        const VkPhysicalDeviceFeatures& supported_device_features)
{
        if (there_is_intersection(required_features, optional_features))
        {
                error("Required and optional physical device features intersect");
        }

        VkPhysicalDeviceFeatures device_features = {};

        try
        {
                set_features(required_features, true, supported_device_features, &device_features);
        }
        catch (const FeatureIsNotSupported& e)
        {
                error(std::string("Required physical device feature ") + e.what() + " is not supported");
        }

        try
        {
                set_features(optional_features, false, supported_device_features, &device_features);
        }
        catch (const FeatureIsNotSupported&)
        {
                error("Exception when setting optional device features");
        }

        return device_features;
}
}

PhysicalDevice::PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
        : m_physical_device(physical_device)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        vkGetPhysicalDeviceProperties(m_physical_device, &m_properties);
        vkGetPhysicalDeviceFeatures(m_physical_device, &m_features);

        m_queue_families = find_queue_families(physical_device);
        m_presentation_supported = find_presentation_support(surface, m_physical_device, m_queue_families);
        m_supported_extensions = find_extensions(m_physical_device);

        ASSERT(m_queue_families.size() == m_presentation_supported.size());
}

PhysicalDevice::operator VkPhysicalDevice() const&
{
        return m_physical_device;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::features() const
{
        return m_features;
}

const VkPhysicalDeviceProperties& PhysicalDevice::properties() const
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
        error("Queue family not found, flags " + to_string(set_flags) + " " + to_string(not_set_flags) + " " +
              to_string(default_flags));
}

uint32_t PhysicalDevice::presentation_family_index() const
{
        for (size_t i = 0; i < m_presentation_supported.size(); ++i)
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
        return std::all_of(extensions.cbegin(), extensions.cend(), [&](const std::string& e) {
                return m_supported_extensions.count(e) >= 1;
        });
}

bool PhysicalDevice::queue_family_supports_presentation(uint32_t index) const
{
        ASSERT(index < m_presentation_supported.size());

        return m_presentation_supported[index];
}

Device PhysicalDevice::create_device(
        const std::unordered_map<uint32_t, uint32_t>& queue_families,
        const std::vector<std::string>& required_extensions,
        const std::vector<PhysicalDeviceFeatures>& required_features,
        const std::vector<PhysicalDeviceFeatures>& optional_features) const
{
        ASSERT(std::all_of(queue_families.cbegin(), queue_families.cend(), [&](const auto& v) {
                return v.first < m_queue_families.size();
        }));
        ASSERT(std::all_of(queue_families.cbegin(), queue_families.cend(), [](const auto& v) { return v.second > 0; }));
        ASSERT(std::all_of(queue_families.cbegin(), queue_families.cend(), [&](const auto& v) {
                return v.second <= m_queue_families[v.first].queueCount;
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

        const VkPhysicalDeviceFeatures enabled_features =
                make_enabled_device_features(required_features, optional_features, m_features);

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &enabled_features;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (!extensions.empty())
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        return Device(m_physical_device, create_info);
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

PhysicalDevice find_physical_device(
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

                if (physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                    physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
                    physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (required_api_version > physical_device.properties().apiVersion)
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
}
