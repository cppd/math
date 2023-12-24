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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

namespace ns::gpu::renderer
{
vulkan::handle::Sampler create_mesh_texture_sampler(const vulkan::Device& device, const bool anisotropy)
{
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;

        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        if (anisotropy)
        {
                if (!device.features().features_10.samplerAnisotropy)
                {
                        error("Sampler anisotropy required but not supported");
                }
                info.anisotropyEnable = VK_TRUE;
                info.maxAnisotropy = 16;
                LOG("Anisotropy enabled");
        }
        else
        {
                info.anisotropyEnable = VK_FALSE;
        }

        info.unnormalizedCoordinates = VK_FALSE;

        info.compareEnable = VK_FALSE;

        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.mipLodBias = 0;
        info.minLod = 0;
        info.maxLod = 0;

        return {device.handle(), info};
}

vulkan::handle::Sampler create_mesh_shadow_sampler(const VkDevice device)
{
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;

        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        info.anisotropyEnable = VK_FALSE;

        info.unnormalizedCoordinates = VK_FALSE;

        info.compareEnable = VK_FALSE;

        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.mipLodBias = 0;
        info.minLod = 0;
        info.maxLod = 0;

        return {device, info};
}
}
