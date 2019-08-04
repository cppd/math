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
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class ConvexHullShaderMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithHostVisibleMemory> m_uniform_buffers;

        struct Data
        {
                Matrix<4, 4, float> matrix;
                float brightness;
        };

        size_t m_data_buffer_index;

public:
        ConvexHullShaderMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        ConvexHullShaderMemory(const ConvexHullShaderMemory&) = delete;
        ConvexHullShaderMemory& operator=(const ConvexHullShaderMemory&) = delete;
        ConvexHullShaderMemory& operator=(ConvexHullShaderMemory&&) = delete;

        ConvexHullShaderMemory(ConvexHullShaderMemory&&) = default;
        ~ConvexHullShaderMemory() = default;

        //

        static unsigned set_number() noexcept;
        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_matrix(const mat4& matrix) const;
        void set_brightness(float brightness) const;
        void set_points(const vulkan::BufferWithMemory& buffer) const;
};
}
