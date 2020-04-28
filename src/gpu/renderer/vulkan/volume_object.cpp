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

#include "volume_object.h"

#include <src/vulkan/buffers.h>

#include <unordered_map>

namespace gpu::renderer
{
namespace
{
// clang-format off
constexpr std::initializer_list<VkFormat> IMAGE_FORMATS =
{
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R32_SFLOAT
};
// clang-format on
}

class VolumeObject::Volume
{
        mat4 m_model_volume_matrix;

        vulkan::ImageWithMemory m_texture;

        std::unordered_map<VkDescriptorSetLayout, VolumeImageMemory> m_memory;

public:
        Volume(const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const vulkan::CommandPool& /*transfer_command_pool*/,
               const vulkan::Queue& /*transfer_queue*/,
               const volume::Volume<3>& volume,
               const mat4& model_matrix)
                : m_model_volume_matrix(model_matrix * volume.matrix),
                  m_texture(
                          device,
                          graphics_command_pool,
                          graphics_queue,
                          {graphics_queue.family_index()},
                          IMAGE_FORMATS,
                          VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_TYPE_3D,
                          vulkan::make_extent(volume.image.size[0], volume.image.size[1], volume.image.size[2]),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          false /*storage*/)
        {
                m_texture.write_linear_grayscale_pixels(
                        graphics_command_pool, graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, volume.image.pixels);
                ASSERT((m_texture.usage() & VK_IMAGE_USAGE_SAMPLED_BIT) == VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT((m_texture.usage() & VK_IMAGE_USAGE_STORAGE_BIT) == 0);
        }

        void create_descriptor_set(const std::function<VolumeImageMemory(VkImageView)>& create)
        {
                VolumeImageMemory memory = create(m_texture.image_view());

                m_memory.erase(memory.descriptor_set_layout());
                m_memory.emplace(memory.descriptor_set_layout(), std::move(memory));
        }

        const VkDescriptorSet& descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const
        {
                auto iter = m_memory.find(descriptor_set_layout);
                if (iter == m_memory.cend())
                {
                        error("Failed to find volume descriptor set for descriptor set layout");
                }

                return iter->second.descriptor_set();
        }

        const mat4& model_matrix() const
        {
                return m_model_volume_matrix;
        }
};

VolumeObject::VolumeObject(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const volume::Volume<3>& volume,
        const mat4& model_matrix)
{
        m_volume = std::make_unique<Volume>(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, volume,
                model_matrix);
}

VolumeObject::~VolumeObject() = default;

void VolumeObject::create_descriptor_set(const std::function<VolumeImageMemory(VkImageView)>& create)
{
        m_volume->create_descriptor_set(create);
}

const VkDescriptorSet& VolumeObject::descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const
{
        return m_volume->descriptor_set(descriptor_set_layout);
}

const mat4& VolumeObject::model_matrix() const
{
        return m_volume->model_matrix();
}
}
