/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/numerical/region.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::convex_hull
{
class PrepareMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int OBJECTS_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        PrepareMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        const VkDescriptorSet& descriptor_set() const;

        void set_object_image(const vulkan::ImageView& storage_image) const;
        void set_lines(const vulkan::Buffer& buffer) const;
};

class PrepareConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                std::int32_t local_size_x;
                std::int32_t buffer_size;
                std::int32_t x;
                std::int32_t y;
                std::int32_t width;
                std::int32_t height;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        PrepareConstant();

        void set(std::int32_t local_size_x, std::int32_t buffer_size, const Region<2, int>& rectangle);
};

class PrepareProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        PrepareConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit PrepareProgram(VkDevice device);

        PrepareProgram(const PrepareProgram&) = delete;
        PrepareProgram& operator=(const PrepareProgram&) = delete;
        PrepareProgram& operator=(PrepareProgram&&) = delete;

        PrepareProgram(PrepareProgram&&) = default;
        ~PrepareProgram() = default;

        void create_pipeline(unsigned buffer_and_group_size, const Region<2, int>& rectangle);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
