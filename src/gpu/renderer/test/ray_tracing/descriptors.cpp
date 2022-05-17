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

#include "descriptors.h"

#include <src/com/error.h>

namespace ns::gpu::renderer::test
{
std::vector<VkDescriptorSetLayoutBinding> RayTracingMemory::descriptor_set_layout_bindings(const bool raygen)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = ACCELERATION_STRUCTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                b.descriptorCount = 1;
                b.stageFlags = raygen ? VK_SHADER_STAGE_RAYGEN_BIT_KHR : VK_SHADER_STAGE_COMPUTE_BIT;
                bindings.push_back(b);
        }

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = raygen ? VK_SHADER_STAGE_RAYGEN_BIT_KHR : VK_SHADER_STAGE_COMPUTE_BIT;
                bindings.push_back(b);
        }

        return bindings;
}

RayTracingMemory::RayTracingMemory(
        const vulkan::Device& device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
}

unsigned RayTracingMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& RayTracingMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void RayTracingMemory::set_acceleration_structure(const VkAccelerationStructureKHR acceleration_structure) const
{
        descriptors_.update_descriptor_set(0, ACCELERATION_STRUCTURE_BINDING, acceleration_structure);
}

void RayTracingMemory::set_image(const vulkan::ImageView& image) const
{
        ASSERT(image.format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        ASSERT(image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image.handle();

        descriptors_.update_descriptor_set(0, IMAGE_BINDING, image_info);
}
}
