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

#pragma once

#include <src/vulkan/buffers.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer
{
class CommonConstants final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t transparency_drawing;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        CommonConstants();

        void set(bool transparency_drawing);
};

class CommonMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;
        static constexpr int SHADOW_BINDING = 2;
        static constexpr int OBJECTS_BINDING = 3;

        static constexpr int TRANSPARENCY_HEADS_BINDING = 4;
        static constexpr int TRANSPARENCY_HEADS_SIZE_BINDING = 5;
        static constexpr int TRANSPARENCY_COUNTERS_BINDING = 6;
        static constexpr int TRANSPARENCY_NODES_BINDING = 7;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(
                VkShaderStageFlags matrices,
                VkShaderStageFlags drawing,
                VkShaderStageFlags shadow,
                VkShaderStageFlags objects);
        static unsigned set_number();

        CommonMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& matrices,
                const vulkan::Buffer& drawing);

        const VkDescriptorSet& descriptor_set() const;

        void set_shadow_texture(VkSampler sampler, const vulkan::DepthImageWithMemory* shadow_texture) const;
        void set_objects_image(const vulkan::ImageWithMemory& objects_image) const;
        void set_transparency(
                const vulkan::ImageWithMemory& heads,
                const vulkan::ImageWithMemory& heads_size,
                const vulkan::Buffer& counters,
                const vulkan::Buffer& nodes) const;
};

class MeshMemory final
{
        static constexpr int SET_NUMBER = 1;
        static constexpr int BUFFER_BINDING = 0;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(VkShaderStageFlags coordinates);
        static unsigned set_number();

        static vulkan::Descriptors create(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const std::vector<const vulkan::Buffer*>& coordinates);
};
}
