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
class DftShowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int IMAGE_BINDING = 1;
        static constexpr int DATA_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Data
        {
                vec4 background_color;
                vec4 foreground_color;
                float brightness;
        };

public:
        DftShowMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        DftShowMemory(const DftShowMemory&) = delete;
        DftShowMemory& operator=(const DftShowMemory&) = delete;
        DftShowMemory& operator=(DftShowMemory&&) = delete;

        DftShowMemory(DftShowMemory&&) = default;
        ~DftShowMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_data(const vec4& background_color, const vec4& foreground_color, float brightness) const;
        void set_image(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
};
}
