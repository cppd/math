/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/matrix.h"
#include "graphics/glsl.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <vector>

namespace gpgpu_vulkan
{
namespace convex_hull_compute_implementation
{
class ShaderMemory
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        // std::vector<vulkan::UniformBufferWithHostVisibleMemory> m_uniform_buffers;
        vulkan::DescriptorSet m_descriptor_set;

        // size_t m_data_buffer_index;

public:
        ShaderMemory(const vulkan::Device& device);

        ShaderMemory(const ShaderMemory&) = delete;
        ShaderMemory& operator=(const ShaderMemory&) = delete;
        ShaderMemory& operator=(ShaderMemory&&) = delete;

        ShaderMemory(ShaderMemory&&) = default;
        ~ShaderMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        VkDescriptorSet descriptor_set() const noexcept;

        //

        void set_object_image(const vulkan::StorageImage& storage_image) const;
        void set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};
}
}
