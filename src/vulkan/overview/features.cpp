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

namespace ns::vulkan
{
namespace
{
void add_feature(
        const VkBool32 value,
        const std::string_view& name,
        std::vector<std::string>* const supported,
        std::vector<std::string>* const not_supported)
{
        (value == VK_TRUE ? supported : not_supported)->emplace_back(name);
}
}

#define ADD_FEATURE_10(v) add_feature(device_features.features_10.v, #v, &supported, &not_supported)

#define ADD_FEATURE_11(v) add_feature(device_features.features_11.v, #v, &supported, &not_supported)

#define ADD_FEATURE_12(v) add_feature(device_features.features_12.v, #v, &supported, &not_supported)

std::tuple<std::vector<std::string>, std::vector<std::string>> find_features(const DeviceFeatures& device_features)
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

        return {std::move(supported), std::move(not_supported)};
}
}
