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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <vector>

namespace gpu_vulkan
{
class PencilSketchComputeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int INPUT_BINDING = 0;
        static constexpr int OUTPUT_BINDING = 1;
        static constexpr int OBJECTS_BINDING = 2;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        //

        PencilSketchComputeMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        PencilSketchComputeMemory(const PencilSketchComputeMemory&) = delete;
        PencilSketchComputeMemory& operator=(const PencilSketchComputeMemory&) = delete;
        PencilSketchComputeMemory& operator=(PencilSketchComputeMemory&&) = delete;

        PencilSketchComputeMemory(PencilSketchComputeMemory&&) = default;
        ~PencilSketchComputeMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_input(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
        void set_output_image(const vulkan::ImageWithMemory& image) const;
        void set_object_image(const vulkan::ImageWithMemory& image) const;
};

class PencilSketchComputeConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t local_size;
                int32_t x;
                int32_t y;
                int32_t width;
                int32_t height;

        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        size_t size() const override;

public:
        PencilSketchComputeConstant();

        void set(int32_t local_size, int32_t x, int32_t y, int32_t width, int32_t height);
};
}
