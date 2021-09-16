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

#pragma once

#include "instance.h"
#include "objects.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/image/format.h>

#include <cstring>
#include <span>
#include <vector>

namespace ns::vulkan
{
enum class BufferMemoryType
{
        HOST_VISIBLE,
        DEVICE_LOCAL
};

class BufferMapper;

class BufferWithMemory final
{
        friend BufferMapper;

        VkDevice device_;
        VkPhysicalDevice physical_device_;
        std::vector<uint32_t> family_indices_;
        Buffer buffer_;
        VkMemoryPropertyFlags memory_properties_;
        DeviceMemory device_memory_;

public:
        BufferWithMemory(
                BufferMemoryType memory_type,
                const Device& device,
                const std::vector<uint32_t>& family_indices,
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
                VkDeviceSize offset,
                VkDeviceSize size,
                const void* data) const;

        void write(const CommandPool& command_pool, const Queue& queue, VkDeviceSize size, const void* data) const;

        const vulkan::Buffer& buffer() const;
        VkDeviceSize size() const;
        bool has_usage(VkBufferUsageFlagBits flag) const;
        VkMemoryPropertyFlags memory_properties() const;
        bool host_visible() const;
};

class BufferMapper final
{
        VkDevice device_;
        VkDeviceMemory device_memory_;
        VkDeviceSize size_;
        void* pointer_;

public:
        BufferMapper(const BufferWithMemory& buffer, VkDeviceSize offset, VkDeviceSize size);
        explicit BufferMapper(const BufferWithMemory& buffer);
        ~BufferMapper();

        BufferMapper(const BufferMapper&) = delete;
        BufferMapper(BufferMapper&&) = delete;
        BufferMapper& operator=(const BufferMapper&) = delete;
        BufferMapper& operator=(BufferMapper&&) = delete;

        template <typename T>
        void write(const T& data) const
        {
                ASSERT(data_size(data) <= size_);
                std::memcpy(pointer_, data_pointer(data), data_size(data));
        }

        template <typename T>
        void write(const VkDeviceSize offset, const T& data) const
        {
                ASSERT(offset + data_size(data) <= size_);
                std::memcpy(static_cast<char*>(pointer_) + offset, data_pointer(data), data_size(data));
        }

        void write(const VkDeviceSize offset, const VkDeviceSize size, const void* const data) const
        {
                ASSERT(offset + size <= size_);
                std::memcpy(static_cast<char*>(pointer_) + offset, data, size);
        }

        template <typename T>
        void read(T* const data) const
        {
                ASSERT(data_size(*data) <= size_);
                std::memcpy(data_pointer(*data), pointer_, data_size(*data));
        }

        template <typename T>
        void read(const VkDeviceSize offset, T* const data) const
        {
                ASSERT(offset + data_size(*data) <= size_);
                std::memcpy(data_pointer(*data), static_cast<const char*>(pointer_) + offset, data_size(*data));
        }
};

template <typename T>
void map_and_write_to_buffer(const BufferWithMemory& buffer, const VkDeviceSize offset, const T& data)
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
void map_and_read_from_buffer(const BufferWithMemory& buffer, const VkDeviceSize offset, T* const data)
{
        BufferMapper map(buffer, offset, data_size(*data));
        map.read(data);
}

template <typename T>
void map_and_read_from_buffer(const BufferWithMemory& buffer, T* const data)
{
        BufferMapper map(buffer, 0, data_size(*data));
        map.read(data);
}

//

inline VkExtent3D make_extent(const uint32_t width, const uint32_t height = 1, const uint32_t depth = 1)
{
        return {.width = width, .height = height, .depth = depth};
}

class ImageWithMemory final
{
        VkExtent3D extent_;
        VkDevice device_;
        VkPhysicalDevice physical_device_;
        std::vector<uint32_t> family_indices_;
        VkImageType type_;
        VkSampleCountFlagBits sample_count_;
        VkImageUsageFlags usage_;
        VkFormat format_;
        Image image_;
        DeviceMemory device_memory_;
        ImageView image_view_;

public:
        ImageWithMemory(
                const Device& device,
                const std::vector<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkSampleCountFlagBits sample_count,
                VkImageType type,
                VkExtent3D extent,
                VkImageUsageFlags usage);

        ImageWithMemory(
                const Device& device,
                const std::vector<uint32_t>& family_indices,
                const std::vector<VkFormat>& format_candidates,
                VkSampleCountFlagBits sample_count,
                VkImageType type,
                VkExtent3D extent,
                VkImageUsageFlags usage,
                VkImageLayout layout,
                const CommandPool& command_pool,
                const Queue& queue);

        ImageWithMemory(const ImageWithMemory&) = delete;
        ImageWithMemory& operator=(const ImageWithMemory&) = delete;
        ImageWithMemory& operator=(ImageWithMemory&&) = delete;

        ImageWithMemory(ImageWithMemory&&) = default;
        ~ImageWithMemory() = default;

        //

        void write_pixels(
                const CommandPool& command_pool,
                const Queue& queue,
                VkImageLayout old_layout,
                VkImageLayout new_layout,
                image::ColorFormat color_format,
                const std::span<const std::byte>& pixels) const;

        void read_pixels(
                const CommandPool& command_pool,
                const Queue& queue,
                VkImageLayout old_layout,
                VkImageLayout new_layout,
                image::ColorFormat* color_format,
                std::vector<std::byte>* pixels) const;

        VkImage image() const;
        VkImageType type() const;
        VkFormat format() const;
        VkImageView image_view() const;
        bool has_usage(VkImageUsageFlags usage) const;
        VkSampleCountFlagBits sample_count() const;
        uint32_t width() const;
        uint32_t height() const;
        uint32_t depth() const;
        VkExtent3D extent() const;
};

class DepthImageWithMemory final
{
        VkImageUsageFlags usage_;
        VkSampleCountFlagBits sample_count_;
        VkFormat format_;
        VkExtent2D extent_;
        Image image_;
        DeviceMemory device_memory_;
        ImageView image_view_;

public:
        DepthImageWithMemory(
                const Device& device,
                const std::vector<uint32_t>& family_indices,
                const std::vector<VkFormat>& formats,
                VkSampleCountFlagBits sample_count,
                uint32_t width,
                uint32_t height,
                VkImageUsageFlags usage);

        DepthImageWithMemory(
                const Device& device,
                const std::vector<uint32_t>& family_indices,
                const std::vector<VkFormat>& formats,
                VkSampleCountFlagBits sample_count,
                uint32_t width,
                uint32_t height,
                VkImageUsageFlags usage,
                VkImageLayout layout,
                VkCommandPool command_pool,
                VkQueue queue);

        DepthImageWithMemory(const DepthImageWithMemory&) = delete;
        DepthImageWithMemory& operator=(const DepthImageWithMemory&) = delete;
        DepthImageWithMemory& operator=(DepthImageWithMemory&&) = delete;

        DepthImageWithMemory(DepthImageWithMemory&&) = default;
        ~DepthImageWithMemory() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;
        VkImageUsageFlags usage() const;
        VkSampleCountFlagBits sample_count() const;

        uint32_t width() const;
        uint32_t height() const;
};
}
