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

        VkDevice m_device;
        VkPhysicalDevice m_physical_device;
        std::unordered_set<uint32_t> m_family_indices;
        Buffer m_buffer;
        VkMemoryPropertyFlags m_memory_properties;
        DeviceMemory m_device_memory;

public:
        BufferWithMemory(
                BufferMemoryType memory_type,
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                VkBufferUsageFlags usage,
                VkDeviceSize size);

        BufferWithMemory(const BufferWithMemory&) = delete;
        BufferWithMemory& operator=(const BufferWithMemory&) = delete;
        BufferWithMemory& operator=(BufferWithMemory&&) = delete;

        BufferWithMemory(BufferWithMemory&&) = default;
        ~BufferWithMemory() = default;

        //

        void write(
                const CommandPool& command_pool,
                const Queue& queue,
                VkDeviceSize data_size,
                const void* data_pointer) const;

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
        VkExtent3D m_extent;
        VkDevice m_device;
        VkPhysicalDevice m_physical_device;
        std::unordered_set<uint32_t> m_family_indices;
        VkImageType m_type;
        VkSampleCountFlagBits m_sample_count;
        VkImageUsageFlags m_usage;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

        void check_family_index(const CommandPool& command_pool, const Queue& queue) const;

public:
        ImageWithMemory(
                const Device& device,
                const CommandPool& command_pool,
                const Queue& queue,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkSampleCountFlagBits sample_count,
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

        void write_srgb_color_pixels(
                const CommandPool& command_pool,
                const Queue& queue,
                VkImageLayout old_layout,
                VkImageLayout new_layout,
                const Span<const std::uint_least8_t>& srgb_pixels) const;

        void write_srgb_grayscale_pixels(
                const CommandPool& command_pool,
                const Queue& queue,
                VkImageLayout old_layout,
                VkImageLayout new_layout,
                const Span<const std::uint_least8_t>& srgb_pixels) const;

        void write_linear_grayscale_pixels(
                const CommandPool& command_pool,
                const Queue& queue,
                VkImageLayout old_layout,
                VkImageLayout new_layout,
                const Span<const std::uint_least16_t>& pixels) const;

        void clear_commands(VkCommandBuffer command_buffer, VkImageLayout image_layout) const;

        VkImage image() const;
        VkImageType type() const;
        VkFormat format() const;
        VkImageView image_view() const;
        VkImageUsageFlags usage() const;
        VkSampleCountFlagBits sample_count() const;
        unsigned width() const;
        unsigned height() const;
        unsigned depth() const;
        VkExtent3D extent() const;
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
