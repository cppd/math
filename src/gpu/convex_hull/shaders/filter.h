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

#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::convex_hull
{
class FilterMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        FilterMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        const VkDescriptorSet& descriptor_set() const;

        void set_lines(const vulkan::Buffer& buffer) const;
        void set_points(const vulkan::Buffer& buffer) const;
        void set_point_count(const vulkan::Buffer& buffer) const;
};

class FilterConstant final : public vulkan::SpecializationConstant
{
        struct Data final
        {
                std::int32_t line_size;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        FilterConstant();

        void set_line_size(std::int32_t v);
};

class FilterProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        FilterConstant constant_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit FilterProgram(VkDevice device);

        FilterProgram(const FilterProgram&) = delete;
        FilterProgram& operator=(const FilterProgram&) = delete;
        FilterProgram& operator=(FilterProgram&&) = delete;

        FilterProgram(FilterProgram&&) = default;
        ~FilterProgram() = default;

        void create_pipeline(unsigned height);
        void delete_pipeline();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline() const;
};
}
