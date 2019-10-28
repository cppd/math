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
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class ConvexHullPrepareMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int OBJECTS_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        ConvexHullPrepareMemory(const vulkan::Device& device);

        ConvexHullPrepareMemory(const ConvexHullPrepareMemory&) = delete;
        ConvexHullPrepareMemory& operator=(const ConvexHullPrepareMemory&) = delete;
        ConvexHullPrepareMemory& operator=(ConvexHullPrepareMemory&&) = delete;

        ConvexHullPrepareMemory(ConvexHullPrepareMemory&&) = default;
        ~ConvexHullPrepareMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_object_image(const vulkan::ImageWithMemory& storage_image) const;
        void set_lines(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullPrepareConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t local_size_x;
                int32_t buffer_size;
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
        ConvexHullPrepareConstant();

        void set(int32_t local_size_x, int32_t buffer_size, int32_t x, int32_t y, int32_t width, int32_t height);
};

class ConvexHullProgramPrepare final
{
        const vulkan::Device& m_device;

        ConvexHullPrepareMemory m_memory;
        ConvexHullPrepareConstant m_constant;
        vulkan::ComputeShader m_shader;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::Pipeline m_pipeline;

        unsigned m_height = 0;

public:
        ConvexHullProgramPrepare(const vulkan::Device& device);

        ConvexHullProgramPrepare(const ConvexHullProgramPrepare&) = delete;
        ConvexHullProgramPrepare& operator=(const ConvexHullProgramPrepare&) = delete;
        ConvexHullProgramPrepare& operator=(ConvexHullProgramPrepare&&) = delete;

        ConvexHullProgramPrepare(ConvexHullProgramPrepare&&) = default;
        ~ConvexHullProgramPrepare() = default;

        void create_buffers(const vulkan::ImageWithMemory& objects, unsigned buffer_and_group_size, unsigned x, unsigned y,
                            unsigned width, unsigned height, const vulkan::BufferWithMemory& lines_buffer);
        void delete_buffers();
        void commands(VkCommandBuffer command_buffer) const;
};
}
