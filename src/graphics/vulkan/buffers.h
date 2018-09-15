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

        void copy(const void* data) const;
};

class TextureImage final
{
        static constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
        static constexpr VkImageLayout IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        Image m_image;
        DeviceMemory m_device_memory;

public:
        TextureImage(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                     VkCommandPool transfer_command_pool, VkQueue transfer_queue, const std::vector<uint32_t>& family_indices,
                     uint32_t width, uint32_t height, const std::vector<unsigned char>& rgba_pixels);

        TextureImage(const TextureImage&) = delete;
        TextureImage& operator=(const TextureImage&) = delete;
        TextureImage& operator=(TextureImage&&) = delete;

        TextureImage(TextureImage&&) = default;
        ~TextureImage() = default;

        //

        operator VkImage() const noexcept;

        VkFormat image_format() const noexcept;
        VkImageLayout image_layout() const noexcept;
};

class DepthImage final
{
        static constexpr VkImageLayout IMAGE_LAYOUT = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkFormat m_image_format;
        Image m_image;
        DeviceMemory m_device_memory;

public:
        DepthImage(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                   const std::vector<uint32_t>& family_indices, uint32_t width, uint32_t height);

        DepthImage(const DepthImage&) = delete;
        DepthImage& operator=(const DepthImage&) = delete;
        DepthImage& operator=(DepthImage&&) = delete;

        DepthImage(DepthImage&&) = default;
        ~DepthImage() = default;

        //

        operator VkImage() const noexcept;

        VkFormat image_format() const noexcept;
        VkImageLayout image_layout() const noexcept;
};
}
