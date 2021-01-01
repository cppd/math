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

#include "objects.h"

#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
enum class PresentMode
{
        PreferSync,
        PreferFast
};

class Swapchain
{
        VkSurfaceFormatKHR m_surface_format;
        VkExtent2D m_extent;
        SwapchainKHR m_swapchain;
        std::vector<VkImage> m_images;
        std::vector<ImageView> m_image_views;

public:
        Swapchain(
                VkSurfaceKHR surface,
                const Device& device,
                const std::unordered_set<uint32_t>& family_indices,
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
        const std::vector<ImageView>& image_views() const;
};

bool acquire_next_image(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore, uint32_t* image_index);
bool queue_present(VkSemaphore wait_semaphore, VkSwapchainKHR swapchain, uint32_t image_index, VkQueue queue);
}
