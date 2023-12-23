/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::optical_flow
{
class GrayscaleMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int SRC_BINDING = 0;
        static constexpr int DST_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        GrayscaleMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set(int index) const;

        void set_src(VkSampler sampler, const vulkan::ImageView& image);
        void set_dst(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1);
};

class GrayscaleProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit GrayscaleProgram(VkDevice device);

        GrayscaleProgram(const GrayscaleProgram&) = delete;
        GrayscaleProgram& operator=(const GrayscaleProgram&) = delete;
        GrayscaleProgram& operator=(GrayscaleProgram&&) = delete;

        GrayscaleProgram(GrayscaleProgram&&) = default;
        ~GrayscaleProgram() = default;

        void create_pipeline(std::uint32_t local_size_x, std::uint32_t local_size_y, const Region<2, int>& rectangle);
        void delete_pipeline();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline() const;
};
}
