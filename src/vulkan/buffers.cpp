/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "buffers.h"

#include "error.h"
#include "objects.h"
#include "strings.h"

#include "buffers/copy.h"
#include "buffers/create.h"
#include "buffers/image.h"
#include "buffers/image_copy.h"
#include "buffers/memory.h"
#include "buffers/query.h"
#include "device/device.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/format.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
namespace
{
// clang-format off
constexpr std::array DEPTH_FORMATS
{
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_X8_D24_UNORM_PACK32
};

constexpr std::array STENCIL_FORMATS
{
        VK_FORMAT_S8_UINT
};
// clang-format on

const std::unordered_set<VkFormat>& depth_format_set()
{
        static const std::unordered_set<VkFormat> formats{DEPTH_FORMATS.cbegin(), DEPTH_FORMATS.cend()};
        return formats;
}

const std::unordered_set<VkFormat>& stencil_format_set()
{
        static const std::unordered_set<VkFormat> formats{STENCIL_FORMATS.cbegin(), STENCIL_FORMATS.cend()};
        return formats;
}

const std::vector<VkFormat>& depth_formats(const std::vector<VkFormat>& formats)
{
        if (!std::all_of(
                    formats.cbegin(), formats.cend(),
                    [depth = &depth_format_set()](const VkFormat format)
                    {
                            return depth->contains(format);
                    }))
        {
                error("Not a depth format: " + strings::formats_to_sorted_string(formats, ", "));
        }

        return formats;
}

const std::vector<VkFormat>& color_formats(const std::vector<VkFormat>& formats)
{
        if (std::any_of(
                    formats.cbegin(), formats.cend(),
                    [depth = &depth_format_set(), stencil = &stencil_format_set()](const VkFormat format)
                    {
                            return depth->contains(format) || stencil->contains(format);
                    }))
        {
                error("Not a color format: " + strings::formats_to_sorted_string(formats, ", "));
        }

        return formats;
}

void check_family_index(
        const CommandPool& command_pool,
        const Queue& queue,
        const std::vector<std::uint32_t>& family_indices)
{
        if (command_pool.family_index() != queue.family_index())
        {
                error("Command pool family index is not equal to queue family index");
        }

        if (!std::binary_search(family_indices.cbegin(), family_indices.cend(), queue.family_index()))
        {
                error("Queue family index is not found in the family indices");
        }
}
}

BufferWithMemory::BufferWithMemory(
        const BufferMemoryType memory_type,
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const VkBufferUsageFlags usage,
        const VkDeviceSize size)
        : physical_device_(device.physical_device()),
          family_indices_(sort_and_unique(family_indices)),
          buffer_(buffers::create_buffer(device.handle(), size, usage, family_indices)),
          memory_properties_(
                  memory_type == BufferMemoryType::HOST_VISIBLE
                          ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                          : (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          device_memory_(buffers::create_device_memory(
                  device.handle(),
                  device.physical_device(),
                  buffer_.handle(),
                  memory_properties_,
                  (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                          ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
                          : 0))
{
        ASSERT(size > 0);
}

void BufferWithMemory::write(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkDeviceSize offset,
        const VkDeviceSize size,
        const void* const data) const
{
        if (offset + size > buffer_.size())
        {
                error("Offset and data size is greater than buffer size");
        }

        ASSERT(data && !host_visible() && buffer_.has_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT));

        check_family_index(command_pool, queue, family_indices_);

        buffers::write_data_to_buffer(
                buffer_.device(), physical_device_, command_pool, queue, buffer_.handle(), offset, size, data);
}

void BufferWithMemory::write(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkDeviceSize size,
        const void* const data) const
{
        if (buffer_.size() != size)
        {
                error("Buffer size and data size are not equal");
        }

        write(command_pool, queue, 0, size, data);
}

const Buffer& BufferWithMemory::buffer() const
{
        return buffer_;
}

VkMemoryPropertyFlags BufferWithMemory::memory_properties() const
{
        return memory_properties_;
}

bool BufferWithMemory::host_visible() const
{
        return (memory_properties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

VkDeviceAddress BufferWithMemory::device_address() const
{
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = buffer_.handle();
        return vkGetBufferDeviceAddress(buffer_.device(), &info);
}

//

BufferMapper::BufferMapper(const BufferWithMemory& buffer, const VkDeviceSize offset, const VkDeviceSize size)
        : device_(buffer.device_memory_.device()),
          device_memory_(buffer.device_memory_),
          size_(size)
{
        ASSERT(buffer.host_visible());
        ASSERT(size_ > 0 && offset + size_ <= buffer.buffer().size());

        VULKAN_CHECK(vkMapMemory(device_, device_memory_, offset, size_, 0, &pointer_));
}

BufferMapper::BufferMapper(const BufferWithMemory& buffer)
        : device_(buffer.device_memory_.device()),
          device_memory_(buffer.device_memory_),
          size_(buffer.buffer().size())
{
        ASSERT(buffer.host_visible());

        VULKAN_CHECK(vkMapMemory(device_, device_memory_, 0, VK_WHOLE_SIZE, 0, &pointer_));
}

BufferMapper::~BufferMapper()
{
        vkUnmapMemory(device_, device_memory_);
        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

//

ImageWithMemory::ImageWithMemory(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        const VkSampleCountFlagBits sample_count,
        const VkImageType type,
        const VkExtent3D extent,
        const VkImageUsageFlags usage)
        : physical_device_(device.physical_device()),
          family_indices_(sort_and_unique(family_indices)),
          image_(buffers::create_image(
                  device.handle(),
                  physical_device_,
                  type,
                  buffers::correct_image_extent(type, extent),
                  buffers::find_supported_image_format(
                          physical_device_,
                          color_formats(formats),
                          type,
                          VK_IMAGE_TILING_OPTIMAL,
                          buffers::format_features_for_image_usage(usage),
                          usage,
                          sample_count),
                  family_indices,
                  sample_count,
                  VK_IMAGE_TILING_OPTIMAL,
                  usage)),
          device_memory_(buffers::create_device_memory(
                  device.handle(),
                  physical_device_,
                  image_.handle(),
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        if (buffers::has_usage_for_image_view(usage))
        {
                image_view_ = buffers::create_image_view(image_, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        else if (!buffers::has_usage_for_transfer(usage))
        {
                error("Unsupported image usage " + to_string_binary(usage) + " for image");
        }
}

ImageWithMemory::ImageWithMemory(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        const VkSampleCountFlagBits sample_count,
        const VkImageType type,
        const VkExtent3D extent,
        const VkImageUsageFlags usage,
        const VkImageLayout layout,
        const CommandPool& command_pool,
        const Queue& queue)
        : ImageWithMemory(device, family_indices, formats, sample_count, type, extent, usage)
{
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                check_family_index(command_pool, queue, family_indices_);

                buffers::transition_image_layout(
                        VK_IMAGE_ASPECT_COLOR_BIT, device.handle(), command_pool.handle(), queue.handle(),
                        image_.handle(), layout);
        }
}

void ImageWithMemory::write(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        const image::ColorFormat color_format,
        const std::span<const std::byte>& pixels) const
{
        ASSERT(image_.has_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT));

        check_family_index(command_pool, queue, family_indices_);

        buffers::write_pixels_to_image(
                image_.device(), physical_device_, command_pool, queue, image_.handle(), image_.format(),
                image_.extent(), VK_IMAGE_ASPECT_COLOR_BIT, old_layout, new_layout, color_format, pixels);
}

void ImageWithMemory::read(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        image::ColorFormat* const color_format,
        std::vector<std::byte>* const pixels) const
{
        ASSERT(image_.has_usage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT));

        check_family_index(command_pool, queue, family_indices_);

        buffers::read_pixels_from_image(
                image_.device(), physical_device_, command_pool, queue, image_.handle(), image_.format(),
                image_.extent(), VK_IMAGE_ASPECT_COLOR_BIT, old_layout, new_layout, color_format, pixels);
}

const Image& ImageWithMemory::image() const
{
        return image_;
}

const ImageView& ImageWithMemory::image_view() const
{
        ASSERT(image_view_.handle() != VK_NULL_HANDLE);
        return image_view_;
}

//

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const VkFormat format,
        const VkSampleCountFlagBits sample_count,
        const std::uint32_t width,
        const std::uint32_t height,
        const VkImageUsageFlags usage)
        : image_(buffers::create_image(
                  device.handle(),
                  device.physical_device(),
                  VK_IMAGE_TYPE_2D,
                  buffers::limit_image_extent(
                          VK_IMAGE_TYPE_2D,
                          make_extent(width, height),
                          device.physical_device(),
                          format,
                          VK_IMAGE_TILING_OPTIMAL,
                          usage),
                  format,
                  family_indices,
                  sample_count,
                  VK_IMAGE_TILING_OPTIMAL,
                  usage)),
          device_memory_(buffers::create_device_memory(
                  device.handle(),
                  device.physical_device(),
                  image_.handle(),
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
                error("Usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT for depth image");
        }

        if (buffers::has_usage_for_image_view(usage))
        {
                image_view_ = buffers::create_image_view(image_, VK_IMAGE_ASPECT_DEPTH_BIT);
        }
        else if (!buffers::has_usage_for_transfer(usage))
        {
                error("Unsupported image usage " + to_string_binary(usage) + " for depth image");
        }
}

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        const VkSampleCountFlagBits sample_count,
        const std::uint32_t width,
        const std::uint32_t height,
        const VkImageUsageFlags usage)
        : DepthImageWithMemory(
                  device,
                  family_indices,
                  buffers::find_supported_image_format(
                          device.physical_device(),
                          depth_formats(formats),
                          VK_IMAGE_TYPE_2D,
                          VK_IMAGE_TILING_OPTIMAL,
                          buffers::format_features_for_image_usage(usage),
                          usage,
                          sample_count),
                  sample_count,
                  width,
                  height,
                  usage)
{
}

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        const VkSampleCountFlagBits sample_count,
        const std::uint32_t width,
        const std::uint32_t height,
        const VkImageUsageFlags usage,
        const VkImageLayout layout,
        const VkCommandPool command_pool,
        const VkQueue queue)
        : DepthImageWithMemory(device, family_indices, formats, sample_count, width, height, usage)
{
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                buffers::transition_image_layout(
                        VK_IMAGE_ASPECT_DEPTH_BIT, device.handle(), command_pool, queue, image_.handle(), layout);
        }
}

const Image& DepthImageWithMemory::image() const
{
        return image_;
}

const ImageView& DepthImageWithMemory::image_view() const
{
        ASSERT(image_view_.handle() != VK_NULL_HANDLE);
        return image_view_;
}
}
