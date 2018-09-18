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
#include "show/renderers/com.h"

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

        struct Matrices
        {
                Matrix<4, 4, float> mvp;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        //

        SharedMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        SharedMemory(const SharedMemory&) = delete;
        SharedMemory& operator=(const SharedMemory&) = delete;
        SharedMemory& operator=(SharedMemory&&) = delete;

        SharedMemory(SharedMemory&&) = default;
        ~SharedMemory() = default;

        //

        VkDescriptorSet descriptor_set() const noexcept;

        void set_matrix(const mat4& matrix) const;
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
                alignas(GLSL_STD140_VEC3_ALIGN) vec3f Ka;
                alignas(GLSL_STD140_VEC3_ALIGN) vec3f Kd;
                alignas(GLSL_STD140_VEC3_ALIGN) vec3f Ks;
                float Ns;
                uint32_t use_texture_Ka;
                uint32_t use_texture_Kd;
                uint32_t use_texture_Ks;
                uint32_t use_material;
        };

        struct MaterialAndTexture
        {
                Material material;
                const vulkan::Texture* texture_Ka;
                const vulkan::Texture* texture_Kd;
                const vulkan::Texture* texture_Ks;
        };

        //

        PerObjectMemory(const vulkan::Device& device, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                        const std::vector<MaterialAndTexture>& materials);

        PerObjectMemory(const PerObjectMemory&) = delete;
        PerObjectMemory& operator=(const PerObjectMemory&) = delete;
        PerObjectMemory& operator=(PerObjectMemory&&) = delete;

        PerObjectMemory(PerObjectMemory&&) = default;
        ~PerObjectMemory() = default;

        //

        uint32_t descriptor_set_count() const noexcept;
        VkDescriptorSet descriptor_set(uint32_t index) const noexcept;
};
}
