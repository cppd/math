/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "texture.h"

namespace
{
vulkan::ImageView create_image_view(VkDevice device, VkImage image, VkFormat format)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return vulkan::ImageView(device, create_info);
}
}

namespace vulkan
{
vulkan::Sampler create_sampler(VkDevice device)
{
        VkSamplerCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;

        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        create_info.anisotropyEnable = VK_TRUE;
        create_info.maxAnisotropy = 16;

        create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

        create_info.unnormalizedCoordinates = VK_FALSE;

        create_info.compareEnable = VK_FALSE;
        create_info.compareOp = VK_COMPARE_OP_ALWAYS;

        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.mipLodBias = 0.0f;
        create_info.minLod = 0.0f;
        create_info.maxLod = 0.0f;

        return vulkan::Sampler(device, create_info);
}

Texture::Texture(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                 VkCommandPool transfer_command_pool, VkQueue transfer_queue, const std::vector<uint32_t>& family_indices,
                 uint32_t width, uint32_t height, const std::vector<unsigned char>& rgba_pixels)
        : m_texture_image(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, family_indices,
                          width, height, rgba_pixels),
          m_image_view(create_image_view(device, m_texture_image, m_texture_image.image_format()))
{
}

VkImageView Texture::image_view() const noexcept
{
        return m_image_view;
}

VkImageLayout Texture::image_layout() const noexcept
{
        return m_texture_image.image_layout();
}
}
