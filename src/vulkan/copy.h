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

#include <vulkan/vulkan.h>

namespace ns::vulkan
{
void commands_image_resolve(
        VkCommandBuffer command_buffer,
        VkPipelineStageFlags src_pipeline_stage_before,
        VkPipelineStageFlags src_pipeline_stage_after,
        VkPipelineStageFlags dst_pipeline_stage_before,
        VkPipelineStageFlags dst_pipeline_stage_after,
        VkAccessFlags src_access_flags_before,
        VkAccessFlags src_access_flags_after,
        VkAccessFlags dst_access_flags_before,
        VkAccessFlags dst_access_flags_after,
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        const Region<2, int>& rectangle);

void commands_image_copy(
        VkCommandBuffer command_buffer,
        VkPipelineStageFlags src_pipeline_stage_before,
        VkPipelineStageFlags src_pipeline_stage_after,
        VkPipelineStageFlags dst_pipeline_stage_before,
        VkPipelineStageFlags dst_pipeline_stage_after,
        VkAccessFlags src_access_flags_before,
        VkAccessFlags src_access_flags_after,
        VkAccessFlags dst_access_flags_before,
        VkAccessFlags dst_access_flags_after,
        VkImageAspectFlags image_aspect_mask,
        VkImage src_image,
        VkImageLayout src_image_layout,
        VkImage dst_image,
        VkImageLayout dst_image_layout,
        const Region<2, int>& rectangle);
}
