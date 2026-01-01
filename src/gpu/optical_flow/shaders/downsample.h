/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::optical_flow
{
class DownsampleMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int BIG_BINDING = 0;
        static constexpr int SMALL_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        DownsampleMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set(int index) const;

        void set_big(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const;
        void set_small(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1) const;
};

class DownsampleProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit DownsampleProgram(VkDevice device);

        DownsampleProgram(const DownsampleProgram&) = delete;
        DownsampleProgram& operator=(const DownsampleProgram&) = delete;
        DownsampleProgram& operator=(DownsampleProgram&&) = delete;

        DownsampleProgram(DownsampleProgram&&) = default;
        ~DownsampleProgram() = default;

        void create_pipeline(std::uint32_t local_size_x, std::uint32_t local_size_y);
        void delete_pipeline();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline() const;
};
}
