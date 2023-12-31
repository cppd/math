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

namespace ns::view
{
void render_buffer_check(
        const std::vector<vulkan::ImageWithMemory>& color,
        const std::vector<vulkan::DepthImageWithMemory>& depth)
{
        if (depth.empty())
        {
                error("No depth attachment");
        }

        if (!std::all_of(
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ImageWithMemory& c)
                    {
                            return c.image().sample_count() == color[0].image().sample_count();
                    }))
        {
                error("Color attachments must have the same sample count");
        }

        if (!std::all_of(
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ImageWithMemory& c)
                    {
                            return c.image().format() == color[0].image().format();
                    }))
        {
                error("Color attachments must have the same format");
        }

        if (!std::all_of(
                    depth.cbegin(), depth.cend(),
                    [&](const vulkan::DepthImageWithMemory& d)
                    {
                            return d.image().sample_count() == depth[0].image().sample_count();
                    }))
        {
                error("Depth attachments must have the same sample count");
        }

        if (!std::all_of(
                    depth.cbegin(), depth.cend(),
                    [&](const vulkan::DepthImageWithMemory& d)
                    {
                            return d.image().format() == depth[0].image().format();
                    }))
        {
                error("Depth attachments must have the same format");
        }

        if (!std::all_of(
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ImageWithMemory& c)
                    {
                            return c.image().sample_count() == depth[0].image().sample_count();
                    }))
        {
                error("Color attachment sample count is not equal to depth attachment sample count");
        }

        if (color.empty())
        {
                if (!std::all_of(
                            depth.cbegin(), depth.cend(),
                            [&](const vulkan::DepthImageWithMemory& d)
                            {
                                    return d.image().sample_count() == VK_SAMPLE_COUNT_1_BIT;
                            }))
                {
                        error("There are no color attachments, but depth attachment sample count is not equal to 1");
                }
        }

        if (!std::all_of(
                    color.cbegin(), color.cend(),
                    [&](const vulkan::ImageWithMemory& d)
                    {
                            return d.image().extent().width == depth[0].image().extent().width
                                   && d.image().extent().height == depth[0].image().extent().height;
                    }))
        {
                error("Color attachments size is not equal to the required size");
        }

        if (!std::all_of(
                    depth.cbegin(), depth.cend(),
                    [&](const vulkan::DepthImageWithMemory& d)
                    {
                            return d.image().extent().width == depth[0].image().extent().width
                                   && d.image().extent().height == depth[0].image().extent().height;
                    }))
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
                    << vulkan::format_to_string(depth[0].image().format());
        }
        else
        {
                oss << "Render buffers do not have depth attachments";
        }

        oss << '\n';
        if (!color.empty())
        {
                oss << "Render buffers color attachment format = "
                    << vulkan::format_to_string(color[0].image().format());
        }
        else
        {
                oss << "Render buffers do not have color attachments";
        }

        return oss.str();
}

}
