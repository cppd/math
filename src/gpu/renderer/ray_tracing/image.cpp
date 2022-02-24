/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "image.h"

#include <src/com/file/path.h>
#include <src/image/alpha.h>
#include <src/image/file_save.h>
#include <src/image/image.h>

#include <filesystem>

namespace ns::gpu::renderer
{
namespace
{
constexpr VkImageLayout IMAGE_LAYOUT = VK_IMAGE_LAYOUT_GENERAL;
}

RayTracingImage::RayTracingImage(
        const unsigned width,
        const unsigned height,
        const vulkan::Device& device,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue)
        : compute_command_pool_(compute_command_pool),
          compute_queue_(compute_queue),
          width_(width),
          height_(height),
          image_(device,
                 {compute_command_pool_->family_index()},
                 {VK_FORMAT_R32G32B32A32_SFLOAT},
                 VK_SAMPLE_COUNT_1_BIT,
                 VK_IMAGE_TYPE_2D,
                 vulkan::make_extent(width_, height_),
                 VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                 IMAGE_LAYOUT,
                 *compute_command_pool_,
                 *compute_queue_)
{
}

void RayTracingImage::save_to_file(const std::string_view& name) const
{
        image::Image<2> res;

        res.size[0] = width_;
        res.size[1] = height_;

        image_.read_pixels(
                *compute_command_pool_, *compute_queue_, IMAGE_LAYOUT, IMAGE_LAYOUT, &res.color_format, &res.pixels);

        res = image::delete_alpha(res);

        image::save(std::filesystem::temp_directory_path() / path_from_utf8(name), image::ImageView<2>(res));
}
}
