/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "ggx_f1_albedo.h"

#include <src/com/merge.h>
#include <src/shading/ggx_f1_albedo.h>

namespace ns::gpu::renderer
{
namespace
{
vulkan::handle::Sampler create_sampler(const VkDevice device)
{
        VkSamplerCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;

        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        create_info.anisotropyEnable = VK_FALSE;

        create_info.unnormalizedCoordinates = VK_FALSE;

        create_info.compareEnable = VK_FALSE;

        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.mipLodBias = 0;
        create_info.minLod = 0;
        create_info.maxLod = 0;

        return vulkan::handle::Sampler(device, create_info);
}

vulkan::ImageWithMemory create_cosine_roughness_image(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue)
{
        const auto [size, data] = shading::ggx_f1_albedo_cosine_roughness_data<3, float>();

        static_assert(std::tuple_size_v<decltype(size)> == 2);
        static_assert(std::is_same_v<decltype(data)::value_type, float>);

        vulkan::ImageWithMemory image(
                device, family_indices, std::vector<VkFormat>({VK_FORMAT_R32_SFLOAT}), VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TYPE_2D, vulkan::make_extent(size[0], size[1]),
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                transfer_command_pool, transfer_queue);

        image.write_pixels(
                transfer_command_pool, transfer_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image::ColorFormat::R32, std::as_bytes(data));

        return image;
}

vulkan::ImageWithMemory create_cosine_weighted_average_image(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue)
{
        const auto [size, data] = shading::ggx_f1_albedo_cosine_weighted_average_data<3, float>();

        static_assert(std::tuple_size_v<decltype(size)> == 1);
        static_assert(std::is_same_v<decltype(data)::value_type, float>);

        vulkan::ImageWithMemory image(
                device, family_indices, std::vector<VkFormat>({VK_FORMAT_R32_SFLOAT}), VK_SAMPLE_COUNT_1_BIT,
                VK_IMAGE_TYPE_1D, vulkan::make_extent(size[0]),
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                transfer_command_pool, transfer_queue);

        image.write_pixels(
                transfer_command_pool, transfer_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image::ColorFormat::R32, std::as_bytes(data));

        return image;
}
}

GgxF1Albedo::GgxF1Albedo(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue)
        : cosine_roughness_(create_cosine_roughness_image(
                device,
                merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_queue.family_index()),
                transfer_command_pool,
                transfer_queue)),
          cosine_weighted_average_(create_cosine_weighted_average_image(
                  device,
                  merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_queue.family_index()),
                  transfer_command_pool,
                  transfer_queue)),
          sampler_(create_sampler(device))
{
}

const vulkan::ImageWithMemory& GgxF1Albedo::cosine_roughness() const
{
        return cosine_roughness_;
}

const vulkan::ImageWithMemory& GgxF1Albedo::cosine_weighted_average() const
{
        return cosine_weighted_average_;
}

const vulkan::handle::Sampler& GgxF1Albedo::sampler() const
{
        return sampler_;
}
}
