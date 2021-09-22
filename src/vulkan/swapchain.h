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

#include "device.h"
#include "objects.h"

#include <optional>
#include <vector>

namespace ns::vulkan
{
enum class PresentMode
{
        PREFER_SYNC,
        PREFER_FAST
};

class Swapchain
{
        VkSurfaceFormatKHR surface_format_;
        VkExtent2D extent_;
        SwapchainKHR swapchain_;
        std::vector<VkImage> images_;
        std::vector<ImageViewHandle> image_view_handles_;
        std::vector<VkImageView> image_views_;

public:
        Swapchain(
                VkSurfaceKHR surface,
                const Device& device,
                const std::vector<uint32_t>& family_indices,
                const VkSurfaceFormatKHR& required_surface_format,
                int preferred_image_count,
                PresentMode preferred_present_mode);

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;

        Swapchain(Swapchain&&) = default;
        ~Swapchain() = default;

        //

        VkSwapchainKHR swapchain() const;

        uint32_t width() const;
        uint32_t height() const;
        VkFormat format() const;
        VkColorSpaceKHR color_space() const;
        const std::vector<VkImageView>& image_views() const;
};

[[nodiscard]] std::optional<uint32_t> acquire_next_image(
        VkDevice device,
        VkSwapchainKHR swapchain,
        VkSemaphore semaphore);

[[nodiscard]] bool queue_present(
        VkSemaphore wait_semaphore,
        VkSwapchainKHR swapchain,
        uint32_t image_index,
        VkQueue queue);
}
