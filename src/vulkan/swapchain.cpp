/*
Copyright (C) 2017-2024 Topological Manifold

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
#include "extensions.h"
#include "objects.h"
#include "strings.h"
#include "surface.h"

#include "device/device.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ns::vulkan
{
namespace
{
const auto* const MULTIPLICATION_SIGN = reinterpret_cast<const char*>(u8"\u00d7");

VkSurfaceFormatKHR choose_surface_format(
        const VkSurfaceFormatKHR required_surface_format,
        const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
        if (surface_formats.empty())
        {
                error("Surface formats not found");
        }

        if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
        {
                return required_surface_format;
        }

        for (const VkSurfaceFormatKHR format : surface_formats)
        {
                if (format.format == required_surface_format.format
                    && format.colorSpace == required_surface_format.colorSpace)
                {
                        return required_surface_format;
                }
        }

        std::string s;
        for (const VkSurfaceFormatKHR format : surface_formats)
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += format_to_string(format.format) + ", " + color_space_to_string(format.colorSpace);
        }
        error("Failed to find surface format " + format_to_string(required_surface_format.format) + ", "
              + color_space_to_string(required_surface_format.colorSpace) + ".\nSupported surface formats:\n" + s);
}

VkPresentModeKHR choose_present_mode(
        const PresentMode preferred_present_mode,
        const std::vector<VkPresentModeKHR>& present_modes)
{
        if (present_modes.empty())
        {
                error("Present modes not found");
        }

        // VK_PRESENT_MODE_FIFO_KHR is required to be supported

        switch (preferred_present_mode)
        {
        case PresentMode::PREFER_SYNC:
                return VK_PRESENT_MODE_FIFO_KHR;
        case PresentMode::PREFER_FAST:
                for (const VkPresentModeKHR present_mode : present_modes)
                {
                        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                        {
                                return present_mode;
                        }
                }
                for (const VkPresentModeKHR present_mode : present_modes)
                {
                        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                        {
                                return present_mode;
                        }
                }
                return VK_PRESENT_MODE_FIFO_KHR;
        }

        error_fatal("Unknown preferred present mode");
}

std::uint32_t choose_image_count(const VkSurfaceCapabilitiesKHR& capabilities, const int image_count)
{
        if (image_count <= 0)
        {
                error("Requested image count " + to_string(image_count) + " must be positive");
        }

        if (static_cast<std::uint32_t>(image_count) <= capabilities.minImageCount)
        {
                return capabilities.minImageCount;
        }

        if (capabilities.maxImageCount > 0 && static_cast<std::uint32_t>(image_count) >= capabilities.maxImageCount)
        {
                return capabilities.maxImageCount;
        }

        return image_count;
}

std::uint32_t find_image_count(const VkDevice device, const VkSwapchainKHR swapchain)
{
        std::uint32_t count;
        VULKAN_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr));
        return count;
}

std::vector<VkImage> swapchain_images(const VkDevice device, const VkSwapchainKHR swapchain)
{
        std::uint32_t image_count = find_image_count(device, swapchain);
        if (image_count < 1)
        {
                error("Failed to find swapchain images");
        }

        std::vector<VkImage> images(image_count);
        VULKAN_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, images.data()));
        return images;
}

handle::SwapchainKHR create_swapchain_khr(
        const VkDevice device,
        const VkSurfaceKHR surface,
        const VkSurfaceFormatKHR surface_format,
        const VkPresentModeKHR present_mode,
        const VkExtent2D extent,
        const std::uint32_t image_count,
        const VkSurfaceTransformFlagBitsKHR transform,
        std::vector<std::uint32_t> family_indices)
{
        if (family_indices.empty())
        {
                error("No swapchain family indices");
        }

        sort_and_unique(&family_indices);

        VkSwapchainCreateInfoKHR info = {};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = surface;
        info.minImageCount = image_count;
        info.imageFormat = surface_format.format;
        info.imageColorSpace = surface_format.colorSpace;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (family_indices.size() > 1)
        {
                info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                info.queueFamilyIndexCount = family_indices.size();
                info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        info.preTransform = transform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = present_mode;
        info.clipped = VK_TRUE;
        info.oldSwapchain = VK_NULL_HANDLE;

        return {device, info};
}

handle::ImageView create_image_view(
        const VkDevice device,
        const VkImage image,
        const VkFormat format,
        const VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = format;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = aspect_flags;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        return {device, info};
}

std::string extent_to_string(const VkExtent2D extent)
{
        return to_string(extent.width) + MULTIPLICATION_SIGN + to_string(extent.height);
}

std::string surface_formats_to_string(const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
        std::string s;
        for (const VkSurfaceFormatKHR format : surface_formats)
        {
                if (!s.empty())
                {
                        s += ", ";
                }
                s += format_to_string(format.format);
        }
        return s;
}

std::string color_spaces_to_string(const std::vector<VkSurfaceFormatKHR>& surface_formats)
{
        std::string s;
        for (const VkSurfaceFormatKHR format : surface_formats)
        {
                if (!s.empty())
                {
                        s += ", ";
                }
                s += color_space_to_string(format.colorSpace);
        }
        return s;
}

std::string present_modes_to_string(const std::vector<VkPresentModeKHR>& present_modes)
{
        std::string s;
        for (const VkPresentModeKHR mode : present_modes)
        {
                if (!s.empty())
                {
                        s += ", ";
                }
                s += present_mode_to_string(mode);
        }
        return s;
}

std::string swapchain_info_string(
        const VkSurfaceCapabilitiesKHR& surface_capabilities,
        const VkExtent2D extent,
        const VkSurfaceFormatKHR surface_format,
        const std::vector<VkSurfaceFormatKHR>& surface_formats,
        const VkPresentModeKHR present_mode,
        const std::vector<VkPresentModeKHR>& present_modes,
        const int preferred_image_count,
        const int image_count)
{
        std::string s;

        s += "Swapchain extent: " + extent_to_string(extent) + " ("
             + extent_to_string(surface_capabilities.minImageExtent) + ", "
             + extent_to_string(surface_capabilities.maxImageExtent) + ")";
        s += '\n';

        s += "Swapchain surface format: " + format_to_string(surface_format.format) + " ("
             + surface_formats_to_string(surface_formats) + ")";
        s += '\n';

        s += "Swapchain color space: " + color_space_to_string(surface_format.colorSpace) + " ("
             + color_spaces_to_string(surface_formats) + ")";
        s += '\n';

        s += "Swapchain present mode: " + present_mode_to_string(present_mode) + " ("
             + present_modes_to_string(present_modes) + ")";
        s += '\n';

        s += "Swapchain preferred image count: " + to_string(preferred_image_count);
        s += '\n';

        s += "Swapchain chosen image count: " + to_string(image_count);

        return s;
}
}

std::optional<std::uint32_t> acquire_next_image(
        const VkDevice device,
        const VkSwapchainKHR swapchain,
        const VkSemaphore semaphore)
{
        static constexpr std::uint64_t TIMEOUT = Limits<std::uint64_t>::max();

        std::uint32_t image_index;
        const VkResult result =
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

        VULKAN_ERROR(result);
}

bool queue_present(
        const VkSemaphore wait_semaphore,
        const VkSwapchainKHR swapchain,
        const std::uint32_t image_index,
        const VkQueue queue)
{
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &wait_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &image_index;
        // present_info.pResults = nullptr;

        const VkResult result = vkQueuePresentKHR(queue, &present_info);

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

        VULKAN_ERROR(result);
}

Swapchain::Swapchain(
        const VkSurfaceKHR surface,
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const VkSurfaceFormatKHR required_surface_format,
        const int preferred_image_count,
        const PresentMode preferred_present_mode)
{
        const VkSurfaceCapabilitiesKHR surface_capabilities =
                find_surface_capabilities(device.physical_device(), surface);

        extent_ = choose_surface_extent(surface_capabilities);

        const std::vector<VkSurfaceFormatKHR> surface_formats = find_surface_formats(device.physical_device(), surface);
        surface_format_ = choose_surface_format(required_surface_format, surface_formats);

        const std::vector<VkPresentModeKHR> present_modes = find_present_modes(device.physical_device(), surface);
        const VkPresentModeKHR present_mode = choose_present_mode(preferred_present_mode, present_modes);

        const std::uint32_t image_count = choose_image_count(surface_capabilities, preferred_image_count);

        LOG(swapchain_info_string(
                surface_capabilities, extent_, surface_format_, surface_formats, present_mode, present_modes,
                preferred_image_count, image_count));

        swapchain_ = create_swapchain_khr(
                device.handle(), surface, surface_format_, present_mode, extent_, image_count,
                surface_capabilities.currentTransform, family_indices);

        images_ = swapchain_images(device.handle(), swapchain_);

        image_view_handles_.reserve(images_.size());
        image_views_.reserve(images_.size());
        for (const VkImage image : images_)
        {
                image_view_handles_.push_back(
                        create_image_view(device.handle(), image, surface_format_.format, VK_IMAGE_ASPECT_COLOR_BIT));
                image_views_.push_back(image_view_handles_.back());
        }
}

VkSwapchainKHR Swapchain::swapchain() const
{
        return swapchain_;
}

std::uint32_t Swapchain::width() const
{
        return extent_.width;
}

std::uint32_t Swapchain::height() const
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

const std::vector<VkImageView>& Swapchain::image_views() const
{
        return image_views_;
}
}
