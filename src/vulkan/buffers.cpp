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

#include "buffers.h"

#include "copy.h"
#include "create.h"
#include "error.h"
#include "memory.h"
#include "print.h"
#include "query.h"
#include "queue.h"

#include <src/com/alg.h>
#include <src/com/print.h>

#include <algorithm>
#include <unordered_set>

namespace ns::vulkan
{
namespace
{
const std::unordered_set<VkFormat>& depth_format_set()
{
        // clang-format off
        static const std::unordered_set<VkFormat> formats
        {
                VK_FORMAT_D16_UNORM,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_X8_D24_UNORM_PACK32
        };
        // clang-format on
        return formats;
}

const std::unordered_set<VkFormat>& stencil_format_set()
{
        // clang-format off
        static const std::unordered_set<VkFormat> formats
        {
                VK_FORMAT_S8_UINT
        };
        // clang-format on
        return formats;
}

VkExtent3D correct_image_extent(const VkImageType& type, const VkExtent3D& extent)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                return {.width = extent.width, .height = 1, .depth = 1};
        case VK_IMAGE_TYPE_2D:
                return {.width = extent.width, .height = extent.height, .depth = 1};
        case VK_IMAGE_TYPE_3D:
                return extent;
        default:
                error("Unknown image type " + image_type_to_string(type));
        }
#pragma GCC diagnostic pop
}

VkExtent3D max_image_extent(
        const VkImageType& type,
        const VkExtent3D& extent,
        const VkPhysicalDevice& physical_device,
        const VkFormat& format,
        const VkImageTiling& tiling,
        const VkImageUsageFlags& usage)
{
        const VkExtent3D max = find_max_image_extent(physical_device, format, type, tiling, usage);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                return {.width = std::min(extent.width, max.width), .height = 1, .depth = 1};
        case VK_IMAGE_TYPE_2D:
                return {.width = std::min(extent.width, max.width),
                        .height = std::min(extent.height, max.height),
                        .depth = 1};
        case VK_IMAGE_TYPE_3D:
                return {.width = std::min(extent.width, max.width),
                        .height = std::min(extent.height, max.height),
                        .depth = std::min(extent.depth, max.depth)};
        default:
                error("Unknown image type " + image_type_to_string(type));
        }
#pragma GCC diagnostic pop
}

void begin_command_buffer(const VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));
}

void end_command_buffer(const VkCommandBuffer command_buffer)
{
        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));
}

void transition_image_layout(
        const VkImageAspectFlags aspect_flags,
        const VkDevice device,
        const VkCommandPool command_pool,
        const VkQueue queue,
        const VkImage image,
        const VkImageLayout layout)
{
        ASSERT(layout != VK_IMAGE_LAYOUT_UNDEFINED);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = aspect_flags;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        const VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        const handle::CommandBuffer command_buffer(device, command_pool);

        begin_command_buffer(command_buffer);

        vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        end_command_buffer(command_buffer);

        queue_submit(command_buffer, queue);
        VULKAN_CHECK(vkQueueWaitIdle(queue));
}

bool has_bits(const VkImageUsageFlags usage, const VkImageUsageFlagBits bits)
{
        return (usage & bits) == bits;
}

VkFormatFeatureFlags format_features_for_image_usage(VkImageUsageFlags usage)
{
        VkFormatFeatureFlags features = 0;
        if (has_bits(usage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (has_bits(usage, VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (has_bits(usage, VK_IMAGE_USAGE_SAMPLED_BIT))
        {
                features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (has_bits(usage, VK_IMAGE_USAGE_STORAGE_BIT))
        {
                features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if (has_bits(usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
        {
                features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (has_bits(usage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
        {
                features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (usage != 0)
        {
                error("Unsupported image usage " + to_string_binary(usage));
        }
        return features;
}

template <typename T>
std::string formats_to_sorted_string(const T& formats, const std::string_view& separator)
{
        static_assert(std::is_same_v<VkFormat, typename T::value_type>);
        if (formats.empty())
        {
                return {};
        }
        std::vector<std::string> v;
        v.reserve(formats.size());
        for (VkFormat format : formats)
        {
                v.push_back(format_to_string(format));
        }
        sort_and_unique(&v);
        auto iter = v.cbegin();
        std::string s = *iter;
        while (++iter != v.cend())
        {
                s += separator;
                s += *iter;
        }
        return s;
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

bool has_usage_for_image_view(const VkImageUsageFlags usage)
{
        return has_bits(usage, VK_IMAGE_USAGE_SAMPLED_BIT) || has_bits(usage, VK_IMAGE_USAGE_STORAGE_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
               || has_bits(usage, VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT);
}

bool has_usage_for_transfer(const VkImageUsageFlags usage)
{
        return has_bits(usage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT) || has_bits(usage, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
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
          buffer_(create_buffer(device, size, usage, family_indices)),
          memory_properties_(
                  memory_type == BufferMemoryType::HOST_VISIBLE
                          ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                          : (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          device_memory_(create_device_memory(device, device.physical_device(), buffer_, memory_properties_))
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

        write_data_to_buffer(buffer_.device(), physical_device_, command_pool, queue, buffer_, offset, size, data);
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

//

BufferMapper::BufferMapper(const BufferWithMemory& buffer, const VkDeviceSize offset, const VkDeviceSize size)
        : device_(buffer.device_memory_.device()), device_memory_(buffer.device_memory_), size_(size)
{
        ASSERT(buffer.host_visible());
        ASSERT(size_ > 0 && offset + size_ <= buffer.buffer().size());

        VULKAN_CHECK(vkMapMemory(device_, device_memory_, offset, size_, 0, &pointer_));
}

BufferMapper::BufferMapper(const BufferWithMemory& buffer)
        : device_(buffer.device_memory_.device()), device_memory_(buffer.device_memory_), size_(buffer.buffer().size())
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
          image_(create_image(
                  device,
                  physical_device_,
                  type,
                  correct_image_extent(type, extent),
                  find_supported_image_format(
                          physical_device_,
                          formats,
                          type,
                          VK_IMAGE_TILING_OPTIMAL,
                          format_features_for_image_usage(usage),
                          usage,
                          sample_count),
                  family_indices,
                  sample_count,
                  VK_IMAGE_TILING_OPTIMAL,
                  usage)),
          device_memory_(create_device_memory(device, physical_device_, image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        if (!std::none_of(
                    formats.cbegin(), formats.cend(),
                    [depth = &depth_format_set(), stencil = &stencil_format_set()](const VkFormat& format)
                    {
                            return depth->contains(format) || stencil->contains(format);
                    }))
        {
                error("Not a color format: " + formats_to_sorted_string(formats, ", "));
        }

        if (has_usage_for_image_view(usage))
        {
                image_view_ = create_image_view(image_, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        else if (!has_usage_for_transfer(usage))
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

                transition_image_layout(VK_IMAGE_ASPECT_COLOR_BIT, device, command_pool, queue, image_, layout);
        }
}

void ImageWithMemory::write_pixels(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        const image::ColorFormat color_format,
        const std::span<const std::byte>& pixels) const
{
        ASSERT(image_.has_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT));

        check_family_index(command_pool, queue, family_indices_);

        write_pixels_to_image(
                image_.device(), physical_device_, command_pool, queue, image_, image_.format(), image_.extent(),
                VK_IMAGE_ASPECT_COLOR_BIT, old_layout, new_layout, color_format, pixels);
}

void ImageWithMemory::read_pixels(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        image::ColorFormat* const color_format,
        std::vector<std::byte>* const pixels) const
{
        ASSERT(image_.has_usage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT));

        check_family_index(command_pool, queue, family_indices_);

        read_pixels_from_image(
                image_.device(), physical_device_, command_pool, queue, image_, image_.format(), image_.extent(),
                VK_IMAGE_ASPECT_COLOR_BIT, old_layout, new_layout, color_format, pixels);
}

const Image& ImageWithMemory::image() const
{
        return image_;
}

const ImageView& ImageWithMemory::image_view() const
{
        ASSERT(static_cast<VkImageView>(image_view_) != VK_NULL_HANDLE);
        return image_view_;
}

//

const std::vector<VkFormat>& DepthImageWithMemory::depth_formats(const std::vector<VkFormat>& formats)
{
        if (!std::all_of(
                    formats.cbegin(), formats.cend(),
                    [depth = &depth_format_set()](const VkFormat& format)
                    {
                            return depth->contains(format);
                    }))
        {
                error("Not a depth format: " + formats_to_sorted_string(formats, ", "));
        }
        return formats;
}

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        VkFormat format,
        VkSampleCountFlagBits sample_count,
        std::uint32_t width,
        std::uint32_t height,
        VkImageUsageFlags usage)
        : image_(create_image(
                device,
                device.physical_device(),
                VK_IMAGE_TYPE_2D,
                max_image_extent(
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
          device_memory_(
                  create_device_memory(device, device.physical_device(), image_, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
                error("Usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT for depth image");
        }

        if (has_usage_for_image_view(usage))
        {
                image_view_ = create_image_view(image_, VK_IMAGE_ASPECT_DEPTH_BIT);
        }
        else if (!has_usage_for_transfer(usage))
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
                find_supported_image_format(
                        device.physical_device(),
                        depth_formats(formats),
                        VK_IMAGE_TYPE_2D,
                        VK_IMAGE_TILING_OPTIMAL,
                        format_features_for_image_usage(usage),
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
                transition_image_layout(VK_IMAGE_ASPECT_DEPTH_BIT, device, command_pool, queue, image_, layout);
        }
}

const Image& DepthImageWithMemory::image() const
{
        return image_;
}

const ImageView& DepthImageWithMemory::image_view() const
{
        ASSERT(static_cast<VkImageView>(image_view_) != VK_NULL_HANDLE);
        return image_view_;
}
}
