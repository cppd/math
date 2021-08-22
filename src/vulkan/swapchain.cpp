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

#include "swapchain.h"

#include "error.h"
#include "print.h"
#include "surface.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

namespace ns::vulkan
{
namespace
{
VkSurfaceFormatKHR choose_surface_format(
        const VkSurfaceFormatKHR& required_surface_format,
        const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
        ASSERT(!surface_formats.empty());

        if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
        {
                return required_surface_format;
        }

        for (const VkSurfaceFormatKHR& format : surface_formats)
        {
                if (format.format == required_surface_format.format
                    && format.colorSpace == required_surface_format.colorSpace)
                {
                        return required_surface_format;
                }
        }

        std::string s;
        for (const VkSurfaceFormatKHR& format : surface_formats)
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += vulkan::format_to_string(format.format) + ", " + vulkan::color_space_to_string(format.colorSpace);
        }
        error("Failed to find surface format " + vulkan::format_to_string(required_surface_format.format) + ", "
              + vulkan::color_space_to_string(required_surface_format.colorSpace) + ".\nSupported surface formats:\n"
              + s);
}

VkPresentModeKHR choose_present_mode(
        const std::vector<VkPresentModeKHR>& present_modes,
        vulkan::PresentMode preferred_present_mode)
{
        switch (preferred_present_mode)
        {
        case vulkan::PresentMode::PreferSync:
                return VK_PRESENT_MODE_FIFO_KHR;
        case vulkan::PresentMode::PreferFast:
                for (const VkPresentModeKHR& present_mode : present_modes)
                {
                        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                        {
                                return present_mode;
                        }
                }
                // VK_PRESENT_MODE_FIFO_KHR is required to be supported
                return VK_PRESENT_MODE_FIFO_KHR;
        }

        error_fatal("Unknown preferred present mode");
}

VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
        if (!(capabilities.currentExtent.width == 0xffff'ffff && capabilities.currentExtent.height == 0xffff'ffff))
        {
                return capabilities.currentExtent;
        }

        error("Current width and height of the surface are not defined");

#if 0
        VkExtent2D extent;
        extent.width = std::clamp(1u, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(1u, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return extent;
#endif
}

uint32_t choose_image_count(const VkSurfaceCapabilitiesKHR& capabilities, int image_count)
{
        if (image_count <= 0)
        {
                error("Requested image count <= 0");
        }

        if (static_cast<uint32_t>(image_count) <= capabilities.minImageCount)
        {
                return capabilities.minImageCount;
        }
        if (capabilities.maxImageCount > 0 && static_cast<uint32_t>(image_count) >= capabilities.maxImageCount)
        {
                return capabilities.maxImageCount;
        }

        return image_count;
}

std::vector<VkImage> swapchain_images(VkDevice device, VkSwapchainKHR swapchain)
{
        uint32_t image_count;
        VkResult result;

        result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetSwapchainImagesKHR", result);
        }

        if (image_count < 1)
        {
                return {};
        }

        std::vector<VkImage> images(image_count);

        result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data());
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetSwapchainImagesKHR", result);
        }

        return images;
}

vulkan::SwapchainKHR create_swapchain_khr(
        VkDevice device,
        VkSurfaceKHR surface,
        VkSurfaceFormatKHR surface_format,
        VkPresentModeKHR present_mode,
        VkExtent2D extent,
        uint32_t image_count,
        VkSurfaceTransformFlagBitsKHR transform,
        std::vector<uint32_t> family_indices)
{
        if (family_indices.empty())
        {
                error("No swapchain family indices");
        }

        sort_and_unique(&family_indices);

        VkSwapchainCreateInfoKHR create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = surface;

        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (family_indices.size() > 1)
        {
                create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = family_indices.size();
                create_info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.preTransform = transform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;

        create_info.oldSwapchain = VK_NULL_HANDLE;

        return vulkan::SwapchainKHR(device, create_info);
}

vulkan::ImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return vulkan::ImageView(device, create_info);
}
std::string swapchain_info_string(const VkSurfaceFormatKHR& surface_format, int preferred_image_count, int image_count)
{
        std::string s;

        s += "Swapchain surface format " + vulkan::format_to_string(surface_format.format);
        s += '\n';
        s += "Swapchain color space " + vulkan::color_space_to_string(surface_format.colorSpace);
        s += '\n';
        s += "Swapchain preferred image count = " + to_string(preferred_image_count);
        s += '\n';
        s += "Swapchain chosen image count = " + to_string(image_count);

        return s;
}
}

std::optional<uint32_t> acquire_next_image(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore semaphore)
{
        static constexpr uint64_t TIMEOUT = limits<uint64_t>::max();

        uint32_t image_index;

        VkResult result =
                vkAcquireNextImageKHR(device, swapchain, TIMEOUT, semaphore, VK_NULL_HANDLE /*fence*/, &image_index);

        if (result == VK_SUCCESS)
        {
                return image_index;
        }
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
                return std::nullopt;
        }
        if (result == VK_SUBOPTIMAL_KHR)
        {
                return image_index;
        }

        vulkan::vulkan_function_error("vkAcquireNextImageKHR", result);
}

bool queue_present(VkSemaphore wait_semaphore, VkSwapchainKHR swapchain, uint32_t image_index, VkQueue queue)
{
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &wait_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;
        // present_info.pResults = nullptr;

        VkResult result = vkQueuePresentKHR(queue, &present_info);

        if (result == VK_SUCCESS)
        {
                return true;
        }
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
                return false;
        }
        if (result == VK_SUBOPTIMAL_KHR)
        {
                return false;
        }

        vulkan::vulkan_function_error("vkQueuePresentKHR", result);
}

Swapchain::Swapchain(
        VkSurfaceKHR surface,
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const VkSurfaceFormatKHR& required_surface_format,
        int preferred_image_count,
        PresentMode preferred_present_mode)
{
        VkSurfaceCapabilitiesKHR surface_capabilities;
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;

        if (!find_surface_details(
                    surface, device.physical_device(), &surface_capabilities, &surface_formats, &present_modes))
        {
                error("Failed to find surface details");
        }

        surface_format_ = choose_surface_format(required_surface_format, surface_formats);
        extent_ = choose_extent(surface_capabilities);
        VkPresentModeKHR present_mode = choose_present_mode(present_modes, preferred_present_mode);
        uint32_t image_count = choose_image_count(surface_capabilities, preferred_image_count);

        LOG(swapchain_info_string(surface_format_, preferred_image_count, image_count));

        swapchain_ = create_swapchain_khr(
                device, surface, surface_format_, present_mode, extent_, image_count,
                surface_capabilities.currentTransform, family_indices);

        images_ = swapchain_images(device, swapchain_);
        if (images_.empty())
        {
                error("Failed to find swapchain images");
        }

        image_views_.reserve(images_.size());
        for (const VkImage& image : images_)
        {
                image_views_.push_back(
                        create_image_view(device, image, surface_format_.format, VK_IMAGE_ASPECT_COLOR_BIT));
        }
}

VkSwapchainKHR Swapchain::swapchain() const
{
        return swapchain_;
}

uint32_t Swapchain::width() const
{
        return extent_.width;
}

uint32_t Swapchain::height() const
{
        return extent_.height;
}

VkFormat Swapchain::format() const
{
        return surface_format_.format;
}

VkColorSpaceKHR Swapchain::color_space() const
{
        return surface_format_.colorSpace;
}

const std::vector<ImageView>& Swapchain::image_views() const
{
        return image_views_;
}
}
