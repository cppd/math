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

#include "instance.h"
#include "objects.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/type/detect.h>

#include <cstring>
#include <unordered_set>

namespace vulkan
{
enum class BufferMemoryType
{
        HostVisible,
        DeviceLocal
};

class BufferMapper;

class BufferWithMemory final
{
        friend BufferMapper;

        Buffer m_buffer;
        VkMemoryPropertyFlags m_memory_properties;
        DeviceMemory m_device_memory;

        void write(VkDeviceSize size, const void* data) const;
        void write(
                const Device& device,
                const CommandPool& transfer_command_pool,
                const Queue& transfer_queue,
                const std::unordered_set<uint32_t>& family_indices,
                VkDeviceSize size,
                const void* data) const;

public:
        BufferWithMemory(
                BufferMemoryType memory_type,
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                VkBufferUsageFlags usage,
                VkDeviceSize size);

        template <typename T>
        BufferWithMemory(
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                VkBufferUsageFlags usage,
                VkDeviceSize size,
                const T& data)
                : BufferWithMemory(BufferMemoryType::HostVisible, device, family_indices, usage, size)
        {
                if (size != data_size(data))
                {
                        error("Buffer size and data size are not equal");
                }
                write(size, data_pointer(data));
        }

        template <typename T>
        BufferWithMemory(
                const Device& device,
                const CommandPool& transfer_command_pool,
                const Queue& transfer_queue,
                const std::unordered_set<uint32_t>& family_indices,
                VkBufferUsageFlags usage,
                VkDeviceSize size,
                const T& data)
                : BufferWithMemory(
                        BufferMemoryType::DeviceLocal,
                        device,
                        family_indices,
                        usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        size)
        {
                if (size != data_size(data))
                {
                        error("Buffer size and data size are not equal");
                }
                write(device, transfer_command_pool, transfer_queue, family_indices, size, data_pointer(data));
        }

        BufferWithMemory(const BufferWithMemory&) = delete;
        BufferWithMemory& operator=(const BufferWithMemory&) = delete;
        BufferWithMemory& operator=(BufferWithMemory&&) = delete;

        BufferWithMemory(BufferWithMemory&&) = default;
        ~BufferWithMemory() = default;

        //

        operator VkBuffer() const&;
        operator VkBuffer() const&& = delete;
        VkDeviceSize size() const;
        const vulkan::Buffer& buffer() const;
        bool usage(VkBufferUsageFlagBits flag) const;
        VkMemoryPropertyFlags memory_properties() const;
        bool host_visible() const;
};

class BufferMapper final
{
        VkDevice m_device;
        VkDeviceMemory m_device_memory;
        unsigned long long m_length;
        void* m_pointer;

public:
        BufferMapper(const BufferWithMemory& buffer, unsigned long long offset, unsigned long long length);
        explicit BufferMapper(const BufferWithMemory& buffer);
        ~BufferMapper();

        BufferMapper(const BufferMapper&) = delete;
        BufferMapper(BufferMapper&&) = delete;
        BufferMapper& operator=(const BufferMapper&) = delete;
        BufferMapper& operator=(BufferMapper&&) = delete;

        template <typename T>
        void write(const T& data) const
        {
                ASSERT(data_size(data) <= m_length);
                std::memcpy(m_pointer, data_pointer(data), data_size(data));
        }

        template <typename T>
        void write(unsigned long long offset, const T& data) const
        {
                ASSERT(offset + data_size(data) <= m_length);
                std::memcpy(static_cast<char*>(m_pointer) + offset, data_pointer(data), data_size(data));
        }

        void write(unsigned long long offset, const void* data, unsigned long long length) const
        {
                ASSERT(offset + length <= m_length);
                std::memcpy(static_cast<char*>(m_pointer) + offset, data, length);
        }

        template <typename T>
        void read(T* data) const
        {
                ASSERT(data_size(*data) <= m_length);
                std::memcpy(data_pointer(*data), m_pointer, data_size(*data));
        }

        template <typename T>
        void read(unsigned long long offset, T* data) const
        {
                ASSERT(offset + data_size(*data) <= m_length);
                std::memcpy(data_pointer(*data), static_cast<const char*>(m_pointer) + offset, data_size(*data));
        }
};

template <typename T>
void map_and_write_to_buffer(const BufferWithMemory& buffer, unsigned long long offset, const T& data)
{
        BufferMapper map(buffer, offset, data_size(data));
        map.write(data);
}

template <typename T>
void map_and_write_to_buffer(const BufferWithMemory& buffer, const T& data)
{
        BufferMapper map(buffer, 0, data_size(data));
        map.write(data);
}

template <typename T>
void map_and_read_from_buffer(const BufferWithMemory& buffer, unsigned long long offset, T* data)
{
        BufferMapper map(buffer, offset, data_size(*data));
        map.read(data);
}

template <typename T>
void map_and_read_from_buffer(const BufferWithMemory& buffer, T* data)
{
        BufferMapper map(buffer, 0, data_size(*data));
        map.read(data);
}

inline VkExtent3D make_extent(unsigned width, unsigned height = 1, unsigned depth = 1)
{
        VkExtent3D extent;
        extent.width = width;
        extent.height = height;
        extent.depth = depth;
        return extent;
}

class ImageWithMemory final
{
        VkImageType m_type;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        VkExtent3D m_extent;
        VkImageUsageFlags m_usage;

        void init(
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkImageType type,
                VkExtent3D extent,
                bool storage,
                VkSampleCountFlagBits samples);

public:
        ImageWithMemory(
                const Device& device,
                const CommandPool& graphics_command_pool,
                const Queue& graphics_queue,
                const CommandPool& transfer_command_pool,
                const Queue& transfer_queue,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkImageType type,
                VkExtent3D extent,
                VkImageLayout image_layout,
                const Span<const std::uint_least8_t>& srgb_pixels,
                bool storage);

        ImageWithMemory(
                const Device& device,
                const CommandPool& graphics_command_pool,
                const Queue& graphics_queue,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkImageType type,
                VkExtent3D extent,
                VkImageLayout image_layout,
                bool storage);

        ImageWithMemory(
                const Device& device,
                const CommandPool& graphics_command_pool,
                const Queue& graphics_queue,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkSampleCountFlagBits samples,
                VkImageType type,
                VkExtent3D extent,
                VkImageLayout image_layout,
                bool storage);

        ImageWithMemory(const ImageWithMemory&) = delete;
        ImageWithMemory& operator=(const ImageWithMemory&) = delete;
        ImageWithMemory& operator=(ImageWithMemory&&) = delete;

        ImageWithMemory(ImageWithMemory&&) = default;
        ~ImageWithMemory() = default;

        //

        VkImage image() const;
        VkImageType type() const;
        VkFormat format() const;
        VkImageView image_view() const;
        VkImageUsageFlags usage() const;

        unsigned width() const;
        unsigned height() const;
        unsigned depth() const;
        VkExtent3D extent() const;

        void clear_commands(VkCommandBuffer command_buffer, VkImageLayout image_layout) const;
};

class DepthAttachment final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        VkSampleCountFlagBits m_sample_count;
        unsigned m_width;
        unsigned m_height;
        VkImageUsageFlags m_usage;

public:
        DepthAttachment(
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& formats,
                VkSampleCountFlagBits samples,
                uint32_t width,
                uint32_t height,
                bool sampled);
        DepthAttachment(
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& formats,
                VkSampleCountFlagBits samples,
                uint32_t width,
                uint32_t height,
                bool sampled,
                VkCommandPool graphics_command_pool,
                VkQueue graphics_queue,
                VkImageLayout image_layout);

        DepthAttachment(const DepthAttachment&) = delete;
        DepthAttachment& operator=(const DepthAttachment&) = delete;
        DepthAttachment& operator=(DepthAttachment&&) = delete;

        DepthAttachment(DepthAttachment&&) = default;
        ~DepthAttachment() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;
        VkImageUsageFlags usage() const;
        VkSampleCountFlagBits sample_count() const;

        unsigned width() const;
        unsigned height() const;
};

class ColorAttachment final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        VkSampleCountFlagBits m_sample_count;

public:
        ColorAttachment(
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                VkFormat format,
                VkSampleCountFlagBits samples,
                uint32_t width,
                uint32_t height);

        ColorAttachment(const ColorAttachment&) = delete;
        ColorAttachment& operator=(const ColorAttachment&) = delete;
        ColorAttachment& operator=(ColorAttachment&&) = delete;

        ColorAttachment(ColorAttachment&&) = default;
        ~ColorAttachment() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;
        VkSampleCountFlagBits sample_count() const;
};
}
