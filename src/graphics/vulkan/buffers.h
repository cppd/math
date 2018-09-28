/*
Copyright (C) 2017, 2018 Topological Manifold

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

namespace vulkan
{
#if 0
class VertexBufferWithHostVisibleMemory final
{
        Buffer m_buffer;
        DeviceMemory m_device_memory;

public:
        VertexBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size, const void* data);

        VertexBufferWithHostVisibleMemory(const VertexBufferWithHostVisibleMemory&) = delete;
        VertexBufferWithHostVisibleMemory& operator=(const VertexBufferWithHostVisibleMemory&) = delete;
        VertexBufferWithHostVisibleMemory& operator=(VertexBufferWithHostVisibleMemory&&) = delete;

        VertexBufferWithHostVisibleMemory(VertexBufferWithHostVisibleMemory&&) = default;
        ~VertexBufferWithHostVisibleMemory() = default;

        //

        operator VkBuffer() const noexcept;
};
#endif

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

class UniformBufferWithHostVisibleMemory
{
        VkDevice m_device;
        VkDeviceSize m_data_size;

        Buffer m_buffer;
        DeviceMemory m_device_memory;

public:
        UniformBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size);

        UniformBufferWithHostVisibleMemory(const UniformBufferWithHostVisibleMemory&) = delete;
        UniformBufferWithHostVisibleMemory& operator=(const UniformBufferWithHostVisibleMemory&) = delete;
        UniformBufferWithHostVisibleMemory& operator=(UniformBufferWithHostVisibleMemory&&) = delete;

        UniformBufferWithHostVisibleMemory(UniformBufferWithHostVisibleMemory&&) = default;
        ~UniformBufferWithHostVisibleMemory() = default;

        //

        operator VkBuffer() const noexcept;

        VkDeviceSize size() const noexcept;

        void copy(VkDeviceSize offset, const void* data, VkDeviceSize data_size) const;
};

class Texture final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

public:
        Texture(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                VkCommandPool transfer_command_pool, VkQueue transfer_queue, const std::vector<uint32_t>& family_indices,
                uint32_t width, uint32_t height, const std::vector<uint16_t>& rgba_pixels);

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) = delete;

        Texture(Texture&&) = default;
        ~Texture() = default;

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

public:
        DepthAttachment(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                        const std::vector<uint32_t>& family_indices, VkSampleCountFlagBits samples, uint32_t width,
                        uint32_t height);

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
};

class ColorAttachment final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

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
};

class ShadowDepthAttachment final
{
        VkImageLayout m_image_layout;
        VkFormat m_format;
        Image m_image;
        DeviceMemory m_device_memory;
        ImageView m_image_view;

public:
        ShadowDepthAttachment(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                              const std::vector<uint32_t>& family_indices, uint32_t* width, uint32_t* height);

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
};
}
