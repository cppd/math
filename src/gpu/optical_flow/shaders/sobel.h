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

#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::optical_flow
{
class SobelMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int I_BINDING = 0;
        static constexpr int DX_BINDING = 1;
        static constexpr int DY_BINDING = 2;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        SobelMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set(int index) const;

        void set_i(const vulkan::ImageView& image_0, const vulkan::ImageView& image_1);
        void set_dx(const vulkan::ImageView& image);
        void set_dy(const vulkan::ImageView& image);
};

class SobelProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit SobelProgram(VkDevice device);

        SobelProgram(const SobelProgram&) = delete;
        SobelProgram& operator=(const SobelProgram&) = delete;
        SobelProgram& operator=(SobelProgram&&) = delete;

        SobelProgram(SobelProgram&&) = default;
        ~SobelProgram() = default;

        void create_pipeline(std::uint32_t local_size_x, std::uint32_t local_size_y);
        void delete_pipeline();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline() const;
};
}
