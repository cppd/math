/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/mat.h"
#include "com/mat_alg.h"
#include "com/vec.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/texture.h"

#include <vector>

namespace vulkan_renderer_shaders
{
struct Vertex
{
        vec3f position;
        vec3f normal;
        vec2f texture_coordinates;

        constexpr Vertex(const vec3f& position_, const vec3f& normal_, const vec2f& texture_coordinates_)
                : position(position_), normal(normal_), texture_coordinates(texture_coordinates_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class SharedMemory
{
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::UniformBufferWithHostVisibleMemory> m_uniform_buffers;
        vulkan::DescriptorSet m_descriptor_set;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        struct Matrices
        {
                Matrix<4, 4, float> mvp;

                Matrices(const mat4& mvp_) : mvp(transpose(to_matrix<float>(mvp_)))
                {
                }
        };

        //

        SharedMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        SharedMemory(const SharedMemory&) = delete;
        SharedMemory& operator=(const SharedMemory&) = delete;
        SharedMemory& operator=(SharedMemory&&) = delete;

        SharedMemory(SharedMemory&&) = default;
        ~SharedMemory() = default;

        //

        VkDescriptorSet descriptor_set() const noexcept;
        void set_uniform(const Matrices& matrices) const;
};

class PerObjectMemory
{
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::UniformBufferWithHostVisibleMemory> m_uniform_buffers;
        std::vector<vulkan::DescriptorSet> m_descriptor_sets;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        struct Material
        {
                float value_r;
                float value_g;
                float value_b;
        };

        //

        PerObjectMemory(const vulkan::Device& device, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                        const std::vector<Material>& materials, const std::vector<const vulkan::Texture*>& textures);

        PerObjectMemory(const PerObjectMemory&) = delete;
        PerObjectMemory& operator=(const PerObjectMemory&) = delete;
        PerObjectMemory& operator=(PerObjectMemory&&) = delete;

        PerObjectMemory(PerObjectMemory&&) = default;
        ~PerObjectMemory() = default;

        //

        VkDescriptorSet descriptor_set(uint32_t index) const noexcept;
};
}
