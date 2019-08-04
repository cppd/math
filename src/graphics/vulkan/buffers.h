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

#include "instance.h"
#include "objects.h"

#include "com/container.h"
#include "com/error.h"
#include "com/type/detect.h"

#include <unordered_set>

namespace vulkan
{
class BufferWithMemory
{
protected:
        BufferWithMemory() = default;
        ~BufferWithMemory() = default;

public:
        BufferWithMemory(const BufferWithMemory&) = default;
        BufferWithMemory& operator=(const BufferWithMemory&) = default;
        BufferWithMemory(BufferWithMemory&&) = default;
        BufferWithMemory& operator=(BufferWithMemory&&) = default;

        virtual operator VkBuffer() const = 0;
        virtual VkDeviceSize size() const = 0;
        virtual bool usage(VkBufferUsageFlagBits flag) const = 0;
};

class BufferWithHostVisibleMemory final : public BufferWithMemory
{
        Buffer m_buffer;
        DeviceMemory m_device_memory;

        void copy_to(VkDeviceSize offset, const void* data, VkDeviceSize data_size) const;
        void copy_from(VkDeviceSize offset, void* data, VkDeviceSize data_size) const;

        BufferWithHostVisibleMemory(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                                    VkBufferUsageFlags usage, VkDeviceSize data_size, const void* data);

public:
        BufferWithHostVisibleMemory(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                                    VkBufferUsageFlags usage, VkDeviceSize data_size);

        template <typename T, typename = std::enable_if_t<sizeof(std::declval<T>().size()) && sizeof(std::declval<T>().data())>>
        explicit BufferWithHostVisibleMemory(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                                             VkBufferUsageFlags usage, const T& data)
                : BufferWithHostVisibleMemory(device, family_indices, usage, storage_size(data), data.data())
        {
        }

        BufferWithHostVisibleMemory(const BufferWithHostVisibleMemory&) = delete;
        BufferWithHostVisibleMemory& operator=(const BufferWithHostVisibleMemory&) = delete;
        BufferWithHostVisibleMemory& operator=(BufferWithHostVisibleMemory&&) = delete;

        BufferWithHostVisibleMemory(BufferWithHostVisibleMemory&&) = default;
        ~BufferWithHostVisibleMemory() = default;

        //

        operator VkBuffer() const override;
        VkDeviceSize size() const override;
        bool usage(VkBufferUsageFlagBits flag) const override;

        //

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> write(const T& data) const
        {
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
                copy_from(0, data->data(), storage_size(*data));
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> read(T* data) const
        {
                ASSERT(size() == sizeof(*data));
                copy_from(0, data, sizeof(*data));
        }
};

class BufferWithDeviceLocalMemory final : public BufferWithMemory
{
        Buffer m_buffer;
        DeviceMemory m_device_memory;

        BufferWithDeviceLocalMemory(const Device& device, const CommandPool& transfer_command_pool, const Queue& transfer_queue,
                                    const std::unordered_set<uint32_t>& family_indices, VkBufferUsageFlags usage,
                                    VkDeviceSize data_size, const void* data);

public:
        BufferWithDeviceLocalMemory(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                                    VkBufferUsageFlags usage, VkDeviceSize data_size);

        template <typename T, typename = std::enable_if_t<sizeof(std::declval<T>().size()) && sizeof(std::declval<T>().data())>>
        explicit BufferWithDeviceLocalMemory(const Device& device, const CommandPool& transfer_command_pool,
                                             const Queue& transfer_queue, const std::unordered_set<uint32_t>& family_indices,
                                             VkBufferUsageFlags usage, const T& data)
                : BufferWithDeviceLocalMemory(device, transfer_command_pool, transfer_queue, family_indices, usage,
                                              storage_size(data), data.data())
        {
        }

        BufferWithDeviceLocalMemory(const BufferWithDeviceLocalMemory&) = delete;
        BufferWithDeviceLocalMemory& operator=(const BufferWithDeviceLocalMemory&) = delete;
        BufferWithDeviceLocalMemory& operator=(BufferWithDeviceLocalMemory&&) = delete;

        BufferWithDeviceLocalMemory(BufferWithDeviceLocalMemory&&) = default;
        ~BufferWithDeviceLocalMemory() = default;

        //

        operator VkBuffer() const override;
        VkDeviceSize size() const override;
        bool usage(VkBufferUsageFlagBits flag) const override;
};

class ColorTexture final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

public:
        ColorTexture(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                     const CommandPool& transfer_command_pool, const Queue& transfer_queue,
                     const std::unordered_set<uint32_t>& family_indices, uint32_t width, uint32_t height,
                     const Span<const std::uint_least8_t>& srgb_uint8_rgba_pixels);

        ColorTexture(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                     const std::unordered_set<uint32_t>& family_indices, VkFormat format, uint32_t width, uint32_t height);

        ColorTexture(const ColorTexture&) = delete;
        ColorTexture& operator=(const ColorTexture&) = delete;
        ColorTexture& operator=(ColorTexture&&) = delete;

        ColorTexture(ColorTexture&&) = default;
        ~ColorTexture() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;
};

class GrayscaleTexture final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

public:
        GrayscaleTexture(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                         const CommandPool& transfer_command_pool, const Queue& transfer_queue,
                         const std::unordered_set<uint32_t>& family_indices, uint32_t width, uint32_t height,
                         const Span<const std::uint_least8_t>& srgb_uint8_grayscale_pixels);

        GrayscaleTexture(const GrayscaleTexture&) = delete;
        GrayscaleTexture& operator=(const GrayscaleTexture&) = delete;
        GrayscaleTexture& operator=(GrayscaleTexture&&) = delete;

        GrayscaleTexture(GrayscaleTexture&&) = default;
        ~GrayscaleTexture() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;
};

class DepthAttachment final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        VkSampleCountFlagBits m_sample_count;
        unsigned m_width, m_height;

public:
        DepthAttachment(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                        const std::vector<VkFormat>& formats, VkSampleCountFlagBits samples, uint32_t width, uint32_t height);

        DepthAttachment(const DepthAttachment&) = delete;
        DepthAttachment& operator=(const DepthAttachment&) = delete;
        DepthAttachment& operator=(DepthAttachment&&) = delete;

        DepthAttachment(DepthAttachment&&) = default;
        ~DepthAttachment() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;
        VkSampleCountFlagBits sample_count() const;

        unsigned width() const;
        unsigned height() const;
};

class DepthAttachmentTexture final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        unsigned m_width, m_height;

public:
        DepthAttachmentTexture(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                               const std::vector<VkFormat>& formats, uint32_t width, uint32_t height);

        DepthAttachmentTexture(const DepthAttachmentTexture&) = delete;
        DepthAttachmentTexture& operator=(const DepthAttachmentTexture&) = delete;
        DepthAttachmentTexture& operator=(DepthAttachmentTexture&&) = delete;

        DepthAttachmentTexture(DepthAttachmentTexture&&) = default;
        ~DepthAttachmentTexture() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;

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
        ColorAttachment(const Device& device, const std::unordered_set<uint32_t>& family_indices, VkFormat format,
                        VkSampleCountFlagBits samples, uint32_t width, uint32_t height);

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

class StorageImage final
{
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;
        unsigned m_width, m_height;

public:
        StorageImage(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                     const std::unordered_set<uint32_t>& family_indices, VkFormat format, uint32_t width, uint32_t height);

        StorageImage(const StorageImage&) = delete;
        StorageImage& operator=(const StorageImage&) = delete;
        StorageImage& operator=(StorageImage&&) = delete;

        StorageImage(StorageImage&&) = default;
        ~StorageImage() = default;

        //

        VkImage image() const;
        VkFormat format() const;
        VkImageView image_view() const;

        unsigned width() const;
        unsigned height() const;

        //

        void clear_commands(VkCommandBuffer command_buffer) const;
};
}
