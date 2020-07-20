/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <src/vulkan/objects.h>

namespace vulkan
{
void commands_resolve(
        VkCommandBuffer command_buffer,
        VkPipelineStageFlags src_stage,
        VkPipelineStageFlags dst_stage,
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        const Region<2, int>& rectangle);
}
