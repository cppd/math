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

#include "sampler.h"

namespace ns::gpu::renderer
{
vulkan::handle::Sampler create_volume_image_sampler(const VkDevice device)
{
        VkSamplerCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;

        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

        create_info.anisotropyEnable = VK_FALSE;

        create_info.unnormalizedCoordinates = VK_FALSE;

        create_info.compareEnable = VK_FALSE;

        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.mipLodBias = 0.0f;
        create_info.minLod = 0.0f;
        create_info.maxLod = 0.0f;

        return {device, create_info};
}

vulkan::handle::Sampler create_volume_depth_image_sampler(const VkDevice device)
{
        VkSamplerCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        create_info.magFilter = VK_FILTER_NEAREST;
        create_info.minFilter = VK_FILTER_NEAREST;

        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        create_info.anisotropyEnable = VK_FALSE;

        create_info.unnormalizedCoordinates = VK_FALSE;

        create_info.compareEnable = VK_FALSE;

        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        create_info.mipLodBias = 0.0f;
        create_info.minLod = 0.0f;
        create_info.maxLod = 0.0f;

        return {device, create_info};
}

vulkan::handle::Sampler create_volume_transfer_function_sampler(const VkDevice device)
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
        create_info.mipLodBias = 0.0f;
        create_info.minLod = 0.0f;
        create_info.maxLod = 0.0f;

        return {device, create_info};
}
}
