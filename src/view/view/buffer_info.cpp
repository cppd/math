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

#include "buffer_info.h"

#include <src/com/error.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/sample.h>
#include <src/vulkan/strings.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace ns::view::view
{
namespace
{
bool check_sample_count(const std::vector<vulkan::ImageWithMemory>& color)
{
        return std::ranges::all_of(
                color,
                [&](const vulkan::ImageWithMemory& c)
                {
                        return c.image().sample_count() == color[0].image().sample_count();
                });
}

bool check_format(const std::vector<vulkan::ImageWithMemory>& color)
{
        return std::ranges::all_of(
                color,
                [&](const vulkan::ImageWithMemory& c)
                {
                        return c.image().format() == color[0].image().format();
                });
}

bool check_sample_count(const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        return std::ranges::all_of(
                depth,
                [&](const vulkan::DepthImageWithMemory& d)
                {
                        return d.image().sample_count() == depth[0].image().sample_count();
                });
}

bool check_format(const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        return std::ranges::all_of(
                depth,
                [&](const vulkan::DepthImageWithMemory& d)
                {
                        return d.image().format() == depth[0].image().format();
                });
}

bool check_sample_count(
        const std::vector<vulkan::ImageWithMemory>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        ASSERT(!depth.empty());

        return std::ranges::all_of(
                color,
                [&](const vulkan::ImageWithMemory& c)
                {
                        return c.image().sample_count() == depth[0].image().sample_count();
                });
}

bool check_sample_count(
        const std::vector<vulkan::DepthImageWithMemory>& depth,
        const VkSampleCountFlagBits sample_count)
{
        return std::ranges::all_of(
                depth,
                [&](const vulkan::DepthImageWithMemory& d)
                {
                        return d.image().sample_count() == sample_count;
                });
}

bool check_color_attachment_sizes(
        const std::vector<vulkan::ImageWithMemory>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        ASSERT(!depth.empty());

        return std::ranges::all_of(
                color,
                [&](const vulkan::ImageWithMemory& d)
                {
                        return d.image().extent().width == depth[0].image().extent().width
                               && d.image().extent().height == depth[0].image().extent().height;
                });
}

bool check_depth_attachment_sizes(const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        return std::ranges::all_of(
                depth,
                [&](const vulkan::DepthImageWithMemory& d)
                {
                        return d.image().extent().width == depth[0].image().extent().width
                               && d.image().extent().height == depth[0].image().extent().height;
                });
}
}

void render_buffer_check(
        const std::vector<vulkan::ImageWithMemory>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!check_sample_count(color))
        {
                error("Color attachments must have the same sample count");
        }

        if (!check_format(color))
        {
                error("Color attachments must have the same format");
        }

        if (!check_sample_count(depth))
        {
                error("Depth attachments must have the same sample count");
        }

        if (!check_format(depth))
        {
                error("Depth attachments must have the same format");
        }

        if (!check_sample_count(color, depth))
        {
                error("Color attachment sample count is not equal to depth attachment sample count");
        }

        if (color.empty())
        {
                if (!check_sample_count(depth, VK_SAMPLE_COUNT_1_BIT))
                {
                        error("There are no color attachments, but depth attachment sample count is not equal to 1");
                }
        }

        if (!check_color_attachment_sizes(color, depth))
        {
                error("Color attachments size is not equal to the required size");
        }

        if (!check_depth_attachment_sizes(depth))
        {
                error("Depth attachments size is not equal to the required size");
        }
}

std::string render_buffer_info(
        const std::vector<vulkan::ImageWithMemory>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        render_buffer_check(color, depth);

        std::ostringstream oss;

        oss << "Render buffers sample count = "
            << vulkan::sample_count_flag_to_sample_count(
                       !color.empty() ? color[0].image().sample_count() : depth[0].image().sample_count());

        oss << '\n';
        if (!depth.empty())
        {
                oss << "Render buffers depth attachment format = "
                    << vulkan::strings::format_to_string(depth[0].image().format());
        }
        else
        {
                oss << "Render buffers do not have depth attachments";
        }

        oss << '\n';
        if (!color.empty())
        {
                oss << "Render buffers color attachment format = "
                    << vulkan::strings::format_to_string(color[0].image().format());
        }
        else
        {
                oss << "Render buffers do not have color attachments";
        }

        return oss.str();
}
}
