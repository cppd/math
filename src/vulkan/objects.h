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

#include "object_handles.h"

#include <src/com/error.h>

#include <cstdint>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
class Instance final
{
        handle::Instance instance_;
        bool validation_layers_enabled_;

public:
        explicit Instance(const VkInstanceCreateInfo& create_info)
                : instance_(create_info), validation_layers_enabled_(create_info.enabledLayerCount > 0)
        {
        }

        operator VkInstance() const& noexcept
        {
                return instance_;
        }
        operator VkInstance() const&& noexcept = delete;

        bool validation_layers_enabled() const noexcept
        {
                return validation_layers_enabled_;
        }
};

class Queue final
{
        VkQueue queue_ = VK_NULL_HANDLE;
        std::uint32_t family_index_ = -1;

public:
        Queue() = default;

        Queue(const std::uint32_t family_index, const VkQueue queue) : queue_(queue), family_index_(family_index)
        {
        }

        operator VkQueue() const& noexcept
        {
                return queue_;
        }
        operator VkQueue() const&& noexcept = delete;

        std::uint32_t family_index() const noexcept
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
                : command_pool_(device, create_info), family_index_(create_info.queueFamilyIndex)
        {
        }

        operator VkCommandPool() const& noexcept
        {
                return command_pool_;
        }
        operator VkCommandPool() const&& noexcept = delete;

        std::uint32_t family_index() const noexcept
        {
                return family_index_;
        }
};

class Buffer final
{
        handle::Buffer buffer_;
        VkDeviceSize size_;
        VkBufferUsageFlags usage_;

public:
        Buffer(const VkDevice device, const VkBufferCreateInfo& create_info)
                : buffer_(device, create_info), size_(create_info.size), usage_(create_info.usage)
        {
        }

        operator VkBuffer() const& noexcept
        {
                return buffer_;
        }
        operator VkBuffer() const&& noexcept = delete;

        VkDevice device() const noexcept
        {
                return buffer_.device();
        }

        VkDeviceSize size() const noexcept
        {
                return size_;
        }

        bool has_usage(const VkBufferUsageFlagBits flag) const noexcept
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

        operator VkImage() const& noexcept
        {
                return image_;
        }
        operator VkImage() const&& noexcept = delete;

        VkDevice device() const noexcept
        {
                return image_.device();
        }

        VkFormat format() const noexcept
        {
                return format_;
        }

        VkExtent3D extent() const noexcept
        {
                return extent_;
        }

        VkImageType type() const noexcept
        {
                return type_;
        }

        VkSampleCountFlagBits sample_count() const noexcept
        {
                return sample_count_;
        }

        bool has_usage(const VkImageUsageFlagBits flag) const noexcept
        {
                return (usage_ & flag) == flag;
        }

        VkImageUsageFlags usage() const noexcept
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
                ASSERT(create_info.pNext == nullptr);
                ASSERT(image == create_info.image);
                ASSERT(image.format() == create_info.format);
        }

        operator VkImageView() const& noexcept
        {
                return image_view_;
        }
        operator VkImageView() const&& noexcept = delete;

        VkFormat format() const noexcept
        {
                return format_;
        }

        VkSampleCountFlagBits sample_count() const noexcept
        {
                return sample_count_;
        }

        bool has_usage(const VkImageUsageFlagBits flag) const noexcept
        {
                return (usage_ & flag) == flag;
        }
};
}
