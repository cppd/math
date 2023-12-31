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

#include "objects.h"

#include "device/device.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace ns::vulkan
{
enum class PresentMode
{
        PREFER_SYNC,
        PREFER_FAST
};

class Swapchain final
{
        VkSurfaceFormatKHR surface_format_;
        VkExtent2D extent_;
        handle::SwapchainKHR swapchain_;
        std::vector<VkImage> images_;
        std::vector<handle::ImageView> image_view_handles_;
        std::vector<VkImageView> image_views_;

public:
        Swapchain(
                VkSurfaceKHR surface,
                const Device& device,
                const std::vector<std::uint32_t>& family_indices,
                VkSurfaceFormatKHR required_surface_format,
                int preferred_image_count,
                PresentMode preferred_present_mode);

        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;
        Swapchain& operator=(Swapchain&&) = delete;

        Swapchain(Swapchain&&) = default;
        ~Swapchain() = default;

        //

        [[nodiscard]] VkSwapchainKHR swapchain() const;

        [[nodiscard]] std::uint32_t width() const;
        [[nodiscard]] std::uint32_t height() const;
        [[nodiscard]] VkFormat format() const;
        [[nodiscard]] VkColorSpaceKHR color_space() const;
        [[nodiscard]] const std::vector<VkImageView>& image_views() const;
};

[[nodiscard]] std::optional<std::uint32_t> acquire_next_image(
        VkDevice device,
        VkSwapchainKHR swapchain,
        VkSemaphore semaphore);

[[nodiscard]] bool queue_present(
        VkSemaphore wait_semaphore,
        VkSwapchainKHR swapchain,
        std::uint32_t image_index,
        VkQueue queue);
}
