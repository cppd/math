/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "object_handles.h" // IWYU pragma: export

#include <src/com/error.h>

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
class Queue final
{
        VkQueue queue_ = VK_NULL_HANDLE;
        std::uint32_t family_index_ = -1;

public:
        Queue() = default;

        Queue(const std::uint32_t family_index, const VkQueue queue)
                : queue_(queue),
                  family_index_(family_index)
        {
        }

        [[nodiscard]] VkQueue handle() const noexcept
        {
                return queue_;
        }

        [[nodiscard]] std::uint32_t family_index() const noexcept
        {
                return family_index_;
        }
};

class CommandPool final
{
        handle::CommandPool command_pool_;
        std::uint32_t family_index_;

public:
        CommandPool(const VkDevice device, const VkCommandPoolCreateInfo& create_info)
                : command_pool_(device, create_info),
                  family_index_(create_info.queueFamilyIndex)
        {
        }

        [[nodiscard]] VkCommandPool handle() const noexcept
        {
                return command_pool_;
        }

        [[nodiscard]] std::uint32_t family_index() const noexcept
        {
                return family_index_;
        }
};

class RenderPass final
{
        handle::RenderPass render_pass_;
        std::uint32_t color_attachment_count_;

public:
        RenderPass() = default;

        RenderPass(const VkDevice device, const VkRenderPassCreateInfo& create_info)
                : render_pass_(device, create_info),
                  color_attachment_count_(create_info.pSubpasses->colorAttachmentCount)
        {
        }

        [[nodiscard]] VkRenderPass handle() const noexcept
        {
                return render_pass_;
        }

        [[nodiscard]] std::uint32_t color_attachment_count() const noexcept
        {
                return color_attachment_count_;
        }
};

class Buffer final
{
        handle::Buffer buffer_;
        VkDeviceSize size_;
        VkBufferUsageFlags usage_;

public:
        Buffer(const VkDevice device, const VkBufferCreateInfo& create_info)
                : buffer_(device, create_info),
                  size_(create_info.size),
                  usage_(create_info.usage)
        {
        }

        [[nodiscard]] VkBuffer handle() const noexcept
        {
                return buffer_;
        }

        [[nodiscard]] VkDevice device() const noexcept
        {
                return buffer_.device();
        }

        [[nodiscard]] VkDeviceSize size() const noexcept
        {
                return size_;
        }

        [[nodiscard]] bool has_usage(const VkBufferUsageFlagBits flag) const noexcept
        {
                return (usage_ & flag) == flag;
        }
};

class Image final
{
        handle::Image image_;
        VkFormat format_;
        VkExtent3D extent_;
        VkImageType type_;
        VkSampleCountFlagBits sample_count_;
        VkImageUsageFlags usage_;

public:
        Image(const VkDevice device, const VkImageCreateInfo& create_info)
                : image_(device, create_info),
                  format_(create_info.format),
                  extent_(create_info.extent),
                  type_(create_info.imageType),
                  sample_count_(create_info.samples),
                  usage_(create_info.usage)
        {
        }

        [[nodiscard]] VkImage handle() const noexcept
        {
                return image_;
        }

        [[nodiscard]] VkDevice device() const noexcept
        {
                return image_.device();
        }

        [[nodiscard]] VkFormat format() const noexcept
        {
                return format_;
        }

        [[nodiscard]] VkExtent3D extent() const noexcept
        {
                return extent_;
        }

        [[nodiscard]] VkImageType type() const noexcept
        {
                return type_;
        }

        [[nodiscard]] VkSampleCountFlagBits sample_count() const noexcept
        {
                return sample_count_;
        }

        [[nodiscard]] bool has_usage(const VkImageUsageFlagBits flag) const noexcept
        {
                return (usage_ & flag) == flag;
        }

        [[nodiscard]] VkImageUsageFlags usage() const noexcept
        {
                return usage_;
        }
};

class ImageView final
{
        handle::ImageView image_view_;
        VkFormat format_;
        VkSampleCountFlagBits sample_count_;
        VkImageUsageFlags usage_;

public:
        ImageView() = default;

        ImageView(const Image& image, const VkImageViewCreateInfo& create_info)
                : image_view_(image.device(), create_info),
                  format_(create_info.format),
                  sample_count_(image.sample_count()),
                  usage_(image.usage())
        {
                ASSERT(!create_info.pNext);
                ASSERT(image.handle() == create_info.image);
                ASSERT(image.format() == create_info.format);
        }

        [[nodiscard]] VkImageView handle() const noexcept
        {
                return image_view_;
        }

        [[nodiscard]] VkFormat format() const noexcept
        {
                return format_;
        }

        [[nodiscard]] VkSampleCountFlagBits sample_count() const noexcept
        {
                return sample_count_;
        }

        [[nodiscard]] bool has_usage(const VkImageUsageFlagBits flag) const noexcept
        {
                return (usage_ & flag) == flag;
        }
};
}
