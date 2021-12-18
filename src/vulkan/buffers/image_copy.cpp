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

#include "image_copy.h"

#include "copy.h"

#include "../print.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/conversion.h>
#include <src/image/swap.h>

namespace ns::vulkan
{
namespace
{
void check_pixel_buffer_size(
        const std::span<const std::byte>& pixels,
        const image::ColorFormat color_format,
        const VkExtent3D extent)
{
        const std::size_t pixel_size = image::format_pixel_size_in_bytes(color_format);

        if (pixels.size() % pixel_size != 0)
        {
                error("Error pixel buffer size " + to_string(pixels.size()) + " for pixel size "
                      + to_string(pixel_size));
        }

        if (pixels.size() != pixel_size * extent.width * extent.height * extent.depth)
        {
                error("Wrong pixel count " + to_string(pixels.size() / pixel_size) + " for image extent ("
                      + to_string(extent.width) + ", " + to_string(extent.height) + ", " + to_string(extent.depth)
                      + ")");
        }
}
}

void write_pixels_to_image(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage image,
        const VkFormat format,
        const VkExtent3D extent,
        const VkImageAspectFlags aspect_flag,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        const image::ColorFormat color_format,
        const std::span<const std::byte>& pixels)
{
        const auto write = [&](const std::span<const std::byte>& data)
        {
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_layout, new_layout, aspect_flag,
                        extent, data);
        };

        check_pixel_buffer_size(pixels, color_format, extent);

        image::ColorFormat required_format;
        bool swap = false;
        bool color = true;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_R8G8B8_SRGB:
                required_format = image::ColorFormat::R8G8B8_SRGB;
                break;
        case VK_FORMAT_B8G8R8_SRGB:
                required_format = image::ColorFormat::R8G8B8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R8G8B8A8_SRGB:
                required_format = image::ColorFormat::R8G8B8A8_SRGB;
                break;
        case VK_FORMAT_B8G8R8A8_SRGB:
                required_format = image::ColorFormat::R8G8B8A8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R16G16B16_UNORM:
                required_format = image::ColorFormat::R16G16B16;
                break;
        case VK_FORMAT_R16G16B16A16_UNORM:
                required_format = image::ColorFormat::R16G16B16A16;
                break;
        case VK_FORMAT_R32G32B32_SFLOAT:
                required_format = image::ColorFormat::R32G32B32;
                break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
                required_format = image::ColorFormat::R32G32B32A32;
                break;
        case VK_FORMAT_R8_SRGB:
                required_format = image::ColorFormat::R8_SRGB;
                break;
        case VK_FORMAT_R16_UNORM:
                required_format = image::ColorFormat::R16;
                break;
        case VK_FORMAT_R32_SFLOAT:
                required_format = image::ColorFormat::R32;
                break;
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string_binary(aspect_flag) + " for writing");
                }
                required_format = image::ColorFormat::R16;
                color = false;
                break;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string_binary(aspect_flag) + " for writing");
                }
                required_format = image::ColorFormat::R32;
                color = false;
                break;
        default:
                error("Unsupported image format " + format_to_string(format) + " for writing");
        }
#pragma GCC diagnostic pop

        if (color && aspect_flag != VK_IMAGE_ASPECT_COLOR_BIT)
        {
                error("Unsupported image aspect " + to_string_binary(aspect_flag) + " for writing");
        }

        if (color_format == required_format)
        {
                if (!swap)
                {
                        write(pixels);
                        return;
                }
                std::vector<std::byte> buffer{pixels.begin(), pixels.end()};
                image::swap_rb(color_format, buffer);
                check_pixel_buffer_size(buffer, color_format, extent);
                write(buffer);
                return;
        }

        std::vector<std::byte> buffer;
        image::format_conversion(color_format, pixels, required_format, &buffer);
        if (swap)
        {
                image::swap_rb(required_format, buffer);
        }
        check_pixel_buffer_size(buffer, required_format, extent);
        write(buffer);
}

void read_pixels_from_image(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage image,
        const VkFormat format,
        const VkExtent3D extent,
        const VkImageAspectFlags aspect_flag,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        image::ColorFormat* const color_format,
        std::vector<std::byte>* const pixels)
{
        bool swap = false;
        bool color = true;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_R8G8B8_SRGB:
                *color_format = image::ColorFormat::R8G8B8_SRGB;
                break;
        case VK_FORMAT_B8G8R8_SRGB:
                *color_format = image::ColorFormat::R8G8B8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R8G8B8A8_SRGB:
                *color_format = image::ColorFormat::R8G8B8A8_SRGB;
                break;
        case VK_FORMAT_B8G8R8A8_SRGB:
                *color_format = image::ColorFormat::R8G8B8A8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R16G16B16_UNORM:
                *color_format = image::ColorFormat::R16G16B16;
                break;
        case VK_FORMAT_R16G16B16A16_UNORM:
                *color_format = image::ColorFormat::R16G16B16A16;
                break;
        case VK_FORMAT_R32G32B32_SFLOAT:
                *color_format = image::ColorFormat::R32G32B32;
                break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
                *color_format = image::ColorFormat::R32G32B32A32;
                break;
        case VK_FORMAT_R8_SRGB:
                *color_format = image::ColorFormat::R8_SRGB;
                break;
        case VK_FORMAT_R16_UNORM:
                *color_format = image::ColorFormat::R16;
                break;
        case VK_FORMAT_R32_SFLOAT:
                *color_format = image::ColorFormat::R32;
                break;
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string_binary(aspect_flag) + " for reading");
                }
                *color_format = image::ColorFormat::R16;
                color = false;
                break;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string_binary(aspect_flag) + " for reading");
                }
                *color_format = image::ColorFormat::R32;
                color = false;
                break;
        default:
                error("Unsupported image format " + format_to_string(format) + " for reading");
        }
#pragma GCC diagnostic pop

        if (color && aspect_flag != VK_IMAGE_ASPECT_COLOR_BIT)
        {
                error("Unsupported image aspect " + to_string_binary(aspect_flag) + " for reading");
        }

        const std::size_t pixel_size = image::format_pixel_size_in_bytes(*color_format);
        const std::size_t size = pixel_size * extent.width * extent.height * extent.depth;

        pixels->resize(size);

        staging_image_read(
                device, physical_device, command_pool, queue, image, old_layout, new_layout, aspect_flag, extent,
                *pixels);

        if (swap)
        {
                image::swap_rb(*color_format, *pixels);
        }
}
}
