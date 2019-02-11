/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "objects.h"

#include "com/container.h"
#include "com/error.h"
#include "com/type/detect.h"

namespace vulkan
{
class VertexBufferWithHostVisibleMemory final
{
        VkDevice m_device;
        VkDeviceSize m_data_size;

        Buffer m_buffer;
        DeviceMemory m_device_memory;

        void copy(VkDeviceSize offset, const void* data, VkDeviceSize data_size) const;

public:
        VertexBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size, const void* data);
        VertexBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size);

        VertexBufferWithHostVisibleMemory(const VertexBufferWithHostVisibleMemory&) = delete;
        VertexBufferWithHostVisibleMemory& operator=(const VertexBufferWithHostVisibleMemory&) = delete;
        VertexBufferWithHostVisibleMemory& operator=(VertexBufferWithHostVisibleMemory&&) = delete;

        VertexBufferWithHostVisibleMemory(VertexBufferWithHostVisibleMemory&&) = default;
        ~VertexBufferWithHostVisibleMemory() = default;

        //

        operator VkBuffer() const noexcept;

        VkDeviceSize size() const noexcept;

        template <typename T>
        void copy(const std::vector<T>& v) const
        {
                copy(0 /*offset*/, v.data(), v.size() * sizeof(T));
        }
};

class VertexBufferWithDeviceLocalMemory final
{
        Buffer m_buffer;
        DeviceMemory m_device_memory;

public:
        VertexBufferWithDeviceLocalMemory(const Device& device, VkCommandPool command_pool, VkQueue queue,
                                          const std::vector<uint32_t>& family_indices, VkDeviceSize data_size, const void* data);

        VertexBufferWithDeviceLocalMemory(const VertexBufferWithDeviceLocalMemory&) = delete;
        VertexBufferWithDeviceLocalMemory& operator=(const VertexBufferWithDeviceLocalMemory&) = delete;
        VertexBufferWithDeviceLocalMemory& operator=(VertexBufferWithDeviceLocalMemory&&) = delete;

        VertexBufferWithDeviceLocalMemory(VertexBufferWithDeviceLocalMemory&&) = default;
        ~VertexBufferWithDeviceLocalMemory() = default;

        //

        operator VkBuffer() const noexcept;
};

class IndexBufferWithDeviceLocalMemory final
{
        Buffer m_buffer;
        DeviceMemory m_device_memory;

public:
        IndexBufferWithDeviceLocalMemory(const Device& device, VkCommandPool command_pool, VkQueue queue,
                                         const std::vector<uint32_t>& family_indices, VkDeviceSize data_size, const void* data);

        IndexBufferWithDeviceLocalMemory(const IndexBufferWithDeviceLocalMemory&) = delete;
        IndexBufferWithDeviceLocalMemory& operator=(const IndexBufferWithDeviceLocalMemory&) = delete;
        IndexBufferWithDeviceLocalMemory& operator=(IndexBufferWithDeviceLocalMemory&&) = delete;

        IndexBufferWithDeviceLocalMemory(IndexBufferWithDeviceLocalMemory&&) = default;
        ~IndexBufferWithDeviceLocalMemory() = default;

        //

        operator VkBuffer() const noexcept;
};

class BufferWithHostVisibleMemory final
{
        VkDevice m_device;
        VkDeviceSize m_data_size;
        VkBufferUsageFlags m_usage;

        Buffer m_buffer;
        DeviceMemory m_device_memory;

        void copy_to(VkDeviceSize offset, const void* data, VkDeviceSize data_size) const;
        void copy_from(VkDeviceSize offset, void* data, VkDeviceSize data_size) const;

public:
        BufferWithHostVisibleMemory(const Device& device, VkBufferUsageFlags usage, VkDeviceSize data_size);

        template <typename T, typename = std::enable_if_t<sizeof(std::declval<T>().size()) && sizeof(std::declval<T>().data())>>
        explicit BufferWithHostVisibleMemory(const Device& device, VkBufferUsageFlags usage, const T& data)
                : BufferWithHostVisibleMemory(device, usage, storage_size(data))
        {
                write(data);
        }

        BufferWithHostVisibleMemory(const BufferWithHostVisibleMemory&) = delete;
        BufferWithHostVisibleMemory& operator=(const BufferWithHostVisibleMemory&) = delete;
        BufferWithHostVisibleMemory& operator=(BufferWithHostVisibleMemory&&) = delete;

        BufferWithHostVisibleMemory(BufferWithHostVisibleMemory&&) = default;
        ~BufferWithHostVisibleMemory() = default;

        //

        operator VkBuffer() const noexcept;
        VkDeviceSize size() const noexcept;
        bool usage(VkBufferUsageFlagBits flag) const noexcept;

        //

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> write(const T& data) const
        {
                ASSERT(size() == storage_size(data));
                copy_to(0, data.data(), storage_size(data));
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> write(const T& data) const
        {
                ASSERT(size() == sizeof(data));
                copy_to(0, &data, sizeof(data));
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> write(VkDeviceSize offset, const T& data) const
        {
                copy_to(offset, data.data(), storage_size(data));
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> write(VkDeviceSize offset, const T& data) const
        {
                copy_to(offset, &data, sizeof(data));
        }

        //

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> read(T* data) const
        {
                ASSERT(size() == storage_size(*data));
                copy_from(0, data->data(), storage_size(*data));
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> read(T* data) const
        {
                ASSERT(size() == sizeof(*data));
                copy_from(0, data, sizeof(*data));
        }
};

class BufferWithDeviceLocalMemory final
{
        VkDevice m_device;
        VkDeviceSize m_data_size;
        VkBufferUsageFlags m_usage;

        Buffer m_buffer;
        DeviceMemory m_device_memory;

public:
        BufferWithDeviceLocalMemory(const Device& device, VkBufferUsageFlags usage, VkDeviceSize data_size);

        BufferWithDeviceLocalMemory(const BufferWithDeviceLocalMemory&) = delete;
        BufferWithDeviceLocalMemory& operator=(const BufferWithDeviceLocalMemory&) = delete;
        BufferWithDeviceLocalMemory& operator=(BufferWithDeviceLocalMemory&&) = delete;

        BufferWithDeviceLocalMemory(BufferWithDeviceLocalMemory&&) = default;
        ~BufferWithDeviceLocalMemory() = default;

        //

        operator VkBuffer() const noexcept;
        VkDeviceSize size() const noexcept;
        bool usage(VkBufferUsageFlagBits flag) const noexcept;
};

class ColorTexture final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

public:
        ColorTexture(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                     VkCommandPool transfer_command_pool, VkQueue transfer_queue, const std::vector<uint32_t>& family_indices,
                     uint32_t width, uint32_t height, const Span<const std::uint_least8_t>& srgb_uint8_rgba_pixels);

        ColorTexture(const ColorTexture&) = delete;
        ColorTexture& operator=(const ColorTexture&) = delete;
        ColorTexture& operator=(ColorTexture&&) = delete;

        ColorTexture(ColorTexture&&) = default;
        ~ColorTexture() = default;

        //

        VkImage image() const noexcept;
        VkFormat format() const noexcept;
        VkImageLayout image_layout() const noexcept;
        VkImageView image_view() const noexcept;
};

class GrayscaleTexture final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

public:
        GrayscaleTexture(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                         VkCommandPool transfer_command_pool, VkQueue transfer_queue, const std::vector<uint32_t>& family_indices,
                         uint32_t width, uint32_t height, const Span<const std::uint_least8_t>& srgb_uint8_grayscale_pixels);

        GrayscaleTexture(const GrayscaleTexture&) = delete;
        GrayscaleTexture& operator=(const GrayscaleTexture&) = delete;
        GrayscaleTexture& operator=(GrayscaleTexture&&) = delete;

        GrayscaleTexture(GrayscaleTexture&&) = default;
        ~GrayscaleTexture() = default;

        //

        VkImage image() const noexcept;
        VkFormat format() const noexcept;
        VkImageLayout image_layout() const noexcept;
        VkImageView image_view() const noexcept;
};

class DepthAttachment final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        unsigned m_width, m_height;

public:
        DepthAttachment(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                        const std::vector<uint32_t>& family_indices, const std::vector<VkFormat>& formats,
                        VkSampleCountFlagBits samples, uint32_t width, uint32_t height);

        DepthAttachment(const DepthAttachment&) = delete;
        DepthAttachment& operator=(const DepthAttachment&) = delete;
        DepthAttachment& operator=(DepthAttachment&&) = delete;

        DepthAttachment(DepthAttachment&&) = default;
        ~DepthAttachment() = default;

        //

        VkImage image() const noexcept;
        VkFormat format() const noexcept;
        VkImageLayout image_layout() const noexcept;
        VkImageView image_view() const noexcept;

        unsigned width() const noexcept;
        unsigned height() const noexcept;
};

class ColorAttachment final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        VkSampleCountFlagBits m_sample_count;

public:
        ColorAttachment(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                        const std::vector<uint32_t>& family_indices, VkFormat format, VkSampleCountFlagBits samples,
                        uint32_t width, uint32_t height);

        ColorAttachment(const ColorAttachment&) = delete;
        ColorAttachment& operator=(const ColorAttachment&) = delete;
        ColorAttachment& operator=(ColorAttachment&&) = delete;

        ColorAttachment(ColorAttachment&&) = default;
        ~ColorAttachment() = default;

        //

        VkImage image() const noexcept;
        VkFormat format() const noexcept;
        VkImageLayout image_layout() const noexcept;
        VkImageView image_view() const noexcept;
        VkSampleCountFlagBits sample_count() const noexcept;
};

class ShadowDepthAttachment final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        unsigned m_width, m_height;

public:
        ShadowDepthAttachment(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                              const std::vector<uint32_t>& family_indices, const std::vector<VkFormat>& formats, uint32_t width,
                              uint32_t height);

        ShadowDepthAttachment(const ShadowDepthAttachment&) = delete;
        ShadowDepthAttachment& operator=(const ShadowDepthAttachment&) = delete;
        ShadowDepthAttachment& operator=(ShadowDepthAttachment&&) = delete;

        ShadowDepthAttachment(ShadowDepthAttachment&&) = default;
        ~ShadowDepthAttachment() = default;

        //

        VkImage image() const noexcept;
        VkFormat format() const noexcept;
        VkImageLayout image_layout() const noexcept;
        VkImageView image_view() const noexcept;

        unsigned width() const noexcept;
        unsigned height() const noexcept;
};

class StorageImage final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        unsigned m_width, m_height;

public:
        StorageImage(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                     const std::vector<uint32_t>& family_indices, VkFormat format, uint32_t width, uint32_t height);

        StorageImage(const StorageImage&) = delete;
        StorageImage& operator=(const StorageImage&) = delete;
        StorageImage& operator=(StorageImage&&) = delete;

        StorageImage(StorageImage&&) = default;
        ~StorageImage() = default;

        //

        VkImage image() const noexcept;
        VkFormat format() const noexcept;
        VkImageLayout image_layout() const noexcept;
        VkImageView image_view() const noexcept;

        unsigned width() const noexcept;
        unsigned height() const noexcept;

        //

        void clear_commands(VkCommandBuffer command_buffer) const;
};

}
