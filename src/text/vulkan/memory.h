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

#include "com/color/color.h"
#include "com/matrix.h"
#include "graphics/glsl.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <unordered_set>
#include <vector>

namespace vulkan_text_implementation
{
class TextMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int TEXTURE_BINDING = 1;
        static constexpr int DRAWING_BINDING = 2;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithHostVisibleMemory> m_uniform_buffers;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

        struct Drawing
        {
                alignas(GLSL_VEC3_ALIGN) vec3f color;
        };

        size_t m_matrices_buffer_index;
        size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        TextMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices, VkSampler sampler,
                   const vulkan::GrayscaleTexture* texture);

        TextMemory(const TextMemory&) = delete;
        TextMemory& operator=(const TextMemory&) = delete;
        TextMemory& operator=(TextMemory&&) = delete;

        TextMemory(TextMemory&&) = default;
        ~TextMemory() = default;

        //

        static unsigned set_number() noexcept;
        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_matrix(const mat4& matrix) const;
        void set_color(const Color& color) const;
};
}
