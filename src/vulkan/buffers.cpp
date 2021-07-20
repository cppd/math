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
#include "sync.h"

#include <src/com/alg.h>

#include <algorithm>
#include <sstream>
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
                VK_FORMAT_S8_UINT,
                VK_FORMAT_X8_D24_UNORM_PACK32
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

void transition_texture_layout(
        const VkImageAspectFlags& aspect_flags,
        const VkDevice& device,
        const VkCommandPool& command_pool,
        const VkQueue& queue,
        const VkImage& image,
        const VkImageLayout& layout)
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

        const CommandBuffer command_buffer(device, command_pool);

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkResult result;

        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkBeginCommandBuffer", result);
        }

        vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEndCommandBuffer", result);
        }

        queue_submit(command_buffer, queue);
        queue_wait_idle(queue);
}

ImageView create_image_view(
        const VkDevice& device,
        const VkImage& image,
        const VkImageType& type,
        const VkFormat& format,
        const VkImageAspectFlags& aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        create_info.image = image;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
                break;
        case VK_IMAGE_TYPE_2D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                break;
        case VK_IMAGE_TYPE_3D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
        default:
                error("Unknown image type " + image_type_to_string(type));
        }
#pragma GCC diagnostic pop

        create_info.format = format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return ImageView(device, create_info);
}

VkFormatFeatureFlags format_features_for_image_usage(VkImageUsageFlags usage)
{
        VkFormatFeatureFlags features = 0;
        if ((usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_SAMPLED_BIT) == VK_IMAGE_USAGE_SAMPLED_BIT)
        {
                features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_STORAGE_BIT) == VK_IMAGE_USAGE_STORAGE_BIT)
        {
                features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
                features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
                features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (usage != 0)
        {
                std::ostringstream oss;
                oss << "Unsupported image usage " << std::hex << usage;
                error(oss.str());
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
        const std::vector<uint32_t>& family_indices)
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
        BufferMemoryType memory_type,
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const VkBufferUsageFlags usage,
        const VkDeviceSize size)
        : m_device(device),
          m_physical_device(device.physical_device()),
          m_family_indices(sort_and_unique(family_indices)),
          m_buffer(create_buffer(device, size, usage, family_indices)),
          m_memory_properties(
                  memory_type == BufferMemoryType::HostVisible
                          ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                          : (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          m_device_memory(create_device_memory(device, device.physical_device(), m_buffer, m_memory_properties))
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
        if (offset + size > m_buffer.size())
        {
                error("Offset and data size is greater than buffer size");
        }

        ASSERT(data && !host_visible() && has_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT));

        check_family_index(command_pool, queue, m_family_indices);

        write_data_to_buffer(m_device, m_physical_device, command_pool, queue, m_buffer, offset, size, data);
}

void BufferWithMemory::write(
        const CommandPool& command_pool,
        const Queue& queue,
        const VkDeviceSize size,
        const void* const data) const
{
        if (m_buffer.size() != size)
        {
                error("Buffer size and data size are not equal");
        }

        write(command_pool, queue, 0, size, data);
}

BufferWithMemory::operator VkBuffer() const&
{
        return m_buffer;
}

const vulkan::Buffer& BufferWithMemory::buffer() const
{
        return m_buffer;
}

VkDeviceSize BufferWithMemory::size() const
{
        return m_buffer.size();
}

bool BufferWithMemory::has_usage(VkBufferUsageFlagBits flag) const
{
        return m_buffer.has_usage(flag);
}

VkMemoryPropertyFlags BufferWithMemory::memory_properties() const
{
        return m_memory_properties;
}

bool BufferWithMemory::host_visible() const
{
        return (m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

//

BufferMapper::BufferMapper(const BufferWithMemory& buffer, const VkDeviceSize offset, const VkDeviceSize size)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_size(size)
{
        ASSERT(buffer.host_visible());
        ASSERT(m_size > 0 && offset + m_size <= buffer.size());

        VkResult result = vkMapMemory(m_device, m_device_memory, offset, m_size, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }
}

BufferMapper::BufferMapper(const BufferWithMemory& buffer)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_size(buffer.size())
{
        ASSERT(buffer.host_visible());

        VkResult result = vkMapMemory(m_device, m_device_memory, 0, VK_WHOLE_SIZE, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }
}

BufferMapper::~BufferMapper()
{
        vkUnmapMemory(m_device, m_device_memory);
        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

//

ImageWithMemory::ImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& format_candidates,
        const VkSampleCountFlagBits sample_count,
        const VkImageType type,
        const VkExtent3D extent,
        const VkImageUsageFlags usage)
        : m_extent(correct_image_extent(type, extent)),
          m_device(device),
          m_physical_device(device.physical_device()),
          m_family_indices(sort_and_unique(family_indices)),
          m_type(type),
          m_sample_count(sample_count),
          m_usage(usage),
          m_format(find_supported_image_format(
                  m_physical_device,
                  format_candidates,
                  m_type,
                  VK_IMAGE_TILING_OPTIMAL,
                  format_features_for_image_usage(m_usage),
                  m_usage,
                  m_sample_count)),
          m_image(create_image(
                  m_device,
                  m_physical_device,
                  m_type,
                  m_extent,
                  m_format,
                  family_indices,
                  m_sample_count,
                  VK_IMAGE_TILING_OPTIMAL,
                  m_usage)),
          m_device_memory(
                  create_device_memory(m_device, m_physical_device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          m_image_view(create_image_view(m_device, m_image, m_type, m_format, VK_IMAGE_ASPECT_COLOR_BIT))
{
        if (!std::none_of(
                    format_candidates.cbegin(), format_candidates.cend(),
                    [set = &depth_format_set()](const VkFormat& format)
                    {
                            return set->contains(format);
                    }))
        {
                error("Depth format found\nformats " + formats_to_sorted_string(format_candidates, ", ")
                      + "\ndepth formats " + formats_to_sorted_string(depth_format_set(), ", "));
        }
}

ImageWithMemory::ImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& format_candidates,
        const VkSampleCountFlagBits sample_count,
        const VkImageType type,
        const VkExtent3D extent,
        const VkImageUsageFlags usage,
        const VkImageLayout layout,
        const CommandPool& command_pool,
        const Queue& queue)
        : ImageWithMemory(device, family_indices, format_candidates, sample_count, type, extent, usage)
{
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                check_family_index(command_pool, queue, m_family_indices);

                transition_texture_layout(VK_IMAGE_ASPECT_COLOR_BIT, device, command_pool, queue, m_image, layout);
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
        ASSERT((m_usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        check_family_index(command_pool, queue, m_family_indices);

        write_pixels_to_image(
                m_device, m_physical_device, command_pool, queue, m_image, m_format, m_extent,
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
        ASSERT((m_usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        check_family_index(command_pool, queue, m_family_indices);

        read_pixels_from_image(
                m_device, m_physical_device, command_pool, queue, m_image, m_format, m_extent,
                VK_IMAGE_ASPECT_COLOR_BIT, old_layout, new_layout, color_format, pixels);
}

VkImage ImageWithMemory::image() const
{
        return m_image;
}

VkImageType ImageWithMemory::type() const
{
        return m_type;
}

VkFormat ImageWithMemory::format() const
{
        return m_format;
}

VkImageView ImageWithMemory::image_view() const
{
        return m_image_view;
}

bool ImageWithMemory::has_usage(const VkImageUsageFlags usage) const
{
        return (m_usage & usage) == usage;
}

VkSampleCountFlagBits ImageWithMemory::sample_count() const
{
        return m_sample_count;
}

uint32_t ImageWithMemory::width() const
{
        return m_extent.width;
}

uint32_t ImageWithMemory::height() const
{
        if (m_type == VK_IMAGE_TYPE_1D)
        {
                error("Image 1D has no height");
        }
        return m_extent.height;
}

uint32_t ImageWithMemory::depth() const
{
        if (m_type != VK_IMAGE_TYPE_3D)
        {
                error("Only image 3D has depth");
        }
        return m_extent.depth;
}

VkExtent3D ImageWithMemory::extent() const
{
        return m_extent;
}

//

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        const VkSampleCountFlagBits sample_count,
        const uint32_t width,
        const uint32_t height,
        const VkImageUsageFlags usage)
{
        if (width <= 0 || height <= 0)
        {
                error("Depth attachment size error");
        }

        if (!std::all_of(
                    formats.cbegin(), formats.cend(),
                    [set = &depth_format_set()](const VkFormat& format)
                    {
                            return set->contains(format);
                    }))
        {
                error("Not only depth formats\nformats " + formats_to_sorted_string(formats, ", ") + "\ndepth formats "
                      + formats_to_sorted_string(depth_format_set(), ", "));
        }

        constexpr VkImageTiling TILING = VK_IMAGE_TILING_OPTIMAL;

        if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
                error("Usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT for depth image");
        }
        m_usage = usage;

        m_format = find_supported_image_format(
                device.physical_device(), formats, VK_IMAGE_TYPE_2D, TILING, format_features_for_image_usage(m_usage),
                m_usage, sample_count);

        const VkExtent3D max_extent =
                max_image_extent(device.physical_device(), m_format, VK_IMAGE_TYPE_2D, TILING, m_usage);
        m_width = std::min(width, max_extent.width);
        m_height = std::min(height, max_extent.height);

        m_image = create_image(
                device, device.physical_device(), VK_IMAGE_TYPE_2D, make_extent(m_width, m_height), m_format,
                family_indices, sample_count, TILING, m_usage);
        m_device_memory =
                create_device_memory(device, device.physical_device(), m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, VK_IMAGE_TYPE_2D, m_format, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_sample_count = sample_count;
}

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        const VkSampleCountFlagBits sample_count,
        const uint32_t width,
        const uint32_t height,
        const VkImageUsageFlags usage,
        const VkImageLayout layout,
        const VkCommandPool command_pool,
        const VkQueue queue)
        : DepthImageWithMemory(device, family_indices, formats, sample_count, width, height, usage)
{
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                transition_texture_layout(VK_IMAGE_ASPECT_DEPTH_BIT, device, command_pool, queue, m_image, layout);
        }
}

VkImage DepthImageWithMemory::image() const
{
        return m_image;
}

VkFormat DepthImageWithMemory::format() const
{
        return m_format;
}

VkImageView DepthImageWithMemory::image_view() const
{
        return m_image_view;
}

VkImageUsageFlags DepthImageWithMemory::usage() const
{
        return m_usage;
}

VkSampleCountFlagBits DepthImageWithMemory::sample_count() const
{
        return m_sample_count;
}

uint32_t DepthImageWithMemory::width() const
{
        return m_width;
}

uint32_t DepthImageWithMemory::height() const
{
        return m_height;
}
}
