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

#include "create.h"

#include "overview.h"
#include "print.h"
#include "query.h"
#include "settings.h"

#include <src/color/conversion.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/vector.h>
#include <src/settings/name.h>

#include <unordered_set>

namespace ns::vulkan
{
namespace
{
void check_image_size(
        const VkPhysicalDevice physical_device,
        const VkImageType type,
        const VkExtent3D extent,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                if (extent.width < 1 || extent.height != 1 || extent.depth != 1)
                {
                        error("Image 1D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                              + to_string(extent.depth) + ")");
                }
                break;
        case VK_IMAGE_TYPE_2D:
                if (extent.width < 1 || extent.height < 1 || extent.depth != 1)
                {
                        error("Image 2D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                              + to_string(extent.depth) + ")");
                }
                break;
        case VK_IMAGE_TYPE_3D:
                if (extent.width < 1 || extent.height < 1 || extent.depth < 1)
                {
                        error("Image 3D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                              + to_string(extent.depth) + ")");
                }
                break;
        default:
                error("Unknown image type " + image_type_to_string(type));
        }
#pragma GCC diagnostic pop

        const VkExtent3D max_extent = find_max_image_extent(physical_device, format, type, tiling, usage);
        if (extent.width > max_extent.width)
        {
                error("Image " + format_to_string(format) + " extent width " + to_string(extent.width)
                      + " is out of range [1, " + to_string(max_extent.width) + "]");
        }
        if (extent.height > max_extent.height)
        {
                error("Image " + format_to_string(format) + " extent height " + to_string(extent.height)
                      + " is out of range [1, " + to_string(max_extent.height) + "]");
        }
        if (extent.depth > max_extent.depth)
        {
                error("Image " + format_to_string(format) + " extent depth " + to_string(extent.depth)
                      + " is out of range [1, " + to_string(max_extent.depth) + "]");
        }
}
}

std::vector<handle::Semaphore> create_semaphores(const VkDevice device, const int count)
{
        std::vector<handle::Semaphore> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device);
        }
        return res;
}

std::vector<handle::Fence> create_fences(const VkDevice device, const int count, const bool signaled_state)
{
        std::vector<handle::Fence> res;
        res.reserve(count);
        for (int i = 0; i < count; ++i)
        {
                res.emplace_back(device, signaled_state);
        }
        return res;
}

//

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
{
        VkPipelineLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.setLayoutCount = descriptor_set_layouts.size();
        create_info.pSetLayouts = descriptor_set_layouts.data();
        // create_info.pushConstantRangeCount = 0;
        // create_info.pPushConstantRanges = nullptr;

        return handle::PipelineLayout(device, create_info);
}

handle::PipelineLayout create_pipeline_layout(
        const VkDevice device,
        const std::vector<unsigned>& set_numbers,
        const std::vector<VkDescriptorSetLayout>& set_layouts)
{
        ASSERT(set_numbers.size() == set_layouts.size() && !set_numbers.empty());
        ASSERT(set_numbers.size() == std::unordered_set<unsigned>(set_numbers.begin(), set_numbers.end()).size());
        ASSERT(0 == *std::min_element(set_numbers.begin(), set_numbers.end()));
        ASSERT(set_numbers.size() - 1 == *std::max_element(set_numbers.begin(), set_numbers.end()));

        std::vector<VkDescriptorSetLayout> layouts(set_numbers.size());
        auto n = set_numbers.begin();
        auto l = set_layouts.begin();
        while (n != set_numbers.end() && l != set_layouts.end())
        {
                layouts.at(*n++) = *l++;
        }

        ASSERT(n == set_numbers.end() && l == set_layouts.end());

        return create_pipeline_layout(device, layouts);
}

//

CommandPool create_command_pool(const VkDevice device, const std::uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;

        return CommandPool(device, create_info);
}

CommandPool create_transient_command_pool(const VkDevice device, const std::uint32_t queue_family_index)
{
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = queue_family_index;
        create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        return CommandPool(device, create_info);
}

//

Instance create_instance(std::vector<std::string> required_extensions)
{
        LOG(overview());

        check_api_version(API_VERSION);

        if (!VALIDATION_LAYERS.empty())
        {
                required_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        sort_and_unique(&required_extensions);

        check_instance_extension_support(required_extensions);

        if (!VALIDATION_LAYERS.empty())
        {
                check_validation_layer_support({VALIDATION_LAYERS.cbegin(), VALIDATION_LAYERS.cend()});
        }

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = settings::APPLICATION_NAME;
        app_info.applicationVersion = 1;
        app_info.pEngineName = nullptr;
        app_info.engineVersion = 0;
        app_info.apiVersion = API_VERSION;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (!extensions.empty())
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        if (!VALIDATION_LAYERS.empty())
        {
                create_info.enabledLayerCount = VALIDATION_LAYERS.size();
                create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
        }

        return Instance(create_info);
}

//

handle::Framebuffer create_framebuffer(
        const VkDevice device,
        const VkRenderPass render_pass,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::vector<VkImageView>& attachments)
{
        VkFramebufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = render_pass;
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        create_info.width = width;
        create_info.height = height;
        create_info.layers = 1;

        return handle::Framebuffer(device, create_info);
}

//

VkClearValue create_color_clear_value(const VkFormat format, const Vector<3, float>& rgb)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        {
                VkClearValue clear_value;
                clear_value.color.float32[0] = color::linear_float_to_srgb_float(rgb[0]);
                clear_value.color.float32[1] = color::linear_float_to_srgb_float(rgb[1]);
                clear_value.color.float32[2] = color::linear_float_to_srgb_float(rgb[2]);
                clear_value.color.float32[3] = 1;
                return clear_value;
        }
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        {
                VkClearValue clear_value;
                clear_value.color.float32[0] = rgb[0];
                clear_value.color.float32[1] = rgb[1];
                clear_value.color.float32[2] = rgb[2];
                clear_value.color.float32[3] = 1;
                return clear_value;
        }
        default:
                error("Unsupported format " + format_to_string(format) + " for color clear value");
        }
#pragma GCC diagnostic pop
}

VkClearValue create_depth_stencil_clear_value()
{
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1;
        clear_value.depthStencil.stencil = 0;
        return clear_value;
}

Buffer create_buffer(
        const VkDevice device,
        const VkDeviceSize size,
        const VkBufferUsageFlags usage,
        std::vector<std::uint32_t> family_indices)
{
        if (size <= 0)
        {
                error("Buffer zero size");
        }

        if (family_indices.empty())
        {
                error("No buffer family indices");
        }

        sort_and_unique(&family_indices);

        VkBufferCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;

        if (family_indices.size() > 1)
        {
                create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = family_indices.size();
                create_info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return Buffer(device, create_info);
}

Image create_image(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const VkImageType type,
        const VkExtent3D extent,
        const VkFormat format,
        std::vector<std::uint32_t> family_indices,
        const VkSampleCountFlagBits samples,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
        check_image_size(physical_device, type, extent, format, tiling, usage);

        if (family_indices.empty())
        {
                error("No image family indices");
        }

        sort_and_unique(&family_indices);

        VkImageCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.imageType = type;
        create_info.extent = extent;
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.format = format;
        create_info.tiling = tiling;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.usage = usage;
        create_info.samples = samples;
        // create_info.flags = 0;

        if (family_indices.size() > 1)
        {
                create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = family_indices.size();
                create_info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return Image(device, create_info);
}

ImageView create_image_view(const Image& image, const VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        create_info.image = image;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (image.type())
        {
        case VK_IMAGE_TYPE_1D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
                break;
        case VK_IMAGE_TYPE_2D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                break;
        case VK_IMAGE_TYPE_3D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
        default:
                error("Unknown image type " + image_type_to_string(image.type()));
        }
#pragma GCC diagnostic pop

        create_info.format = image.format();

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return ImageView(image, create_info);
}
}
