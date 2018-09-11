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

#pragma once

#include "buffers.h"
#include "objects.h"

namespace vulkan
{
vulkan::Sampler create_sampler(VkDevice device);

class Texture
{
        TextureImage m_texture_image;
        ImageView m_image_view;

public:
        Texture(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                VkCommandPool transfer_command_pool, VkQueue transfer_queue, const std::vector<uint32_t>& family_indices,
                uint32_t width, uint32_t height, const std::vector<unsigned char>& rgba_pixels);
};
}
