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

#pragma once

#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer::test
{
class RayTracingMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int ACCELERATION_STRUCTURE_BINDING = 0;
        static constexpr int IMAGE_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(bool raygen);
        static unsigned set_number();

        RayTracingMemory(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings);

        const VkDescriptorSet& descriptor_set() const;

        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure) const;
        void set_image(const vulkan::ImageView& image) const;
};
}
