/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "instance.h"

#include "create.h"
#include "debug.h"
#include "error.h"
#include "instance_create.h"
#include "instance_info.h"
#include "settings.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <tuple>
#include <unordered_map>

namespace ns::vulkan
{
namespace
{
constexpr const char* DEBUG = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
constexpr std::uint32_t NO_FAMILY_INDEX = Limits<std::uint32_t>::max();

std::unordered_set<std::string> make_layers(
        const std::unordered_set<std::string>& required_layers,
        const std::unordered_set<std::string>& optional_layers)
{
        std::unordered_set<std::string> res;

        const std::unordered_set<std::string> supported_layers = supported_instance_layers();

        for (const std::string& layer : required_layers)
        {
                if (!supported_layers.contains(layer))
                {
                        error("Vulkan layer " + layer + " is not supported");
                }
                res.insert(layer);
        }

        for (const char* const layer : LAYERS)
        {
                if (!supported_layers.contains(layer))
                {
                        error(std::string("Vulkan layer ") + layer + " is not supported");
                }
                res.insert(layer);
        }

        for (const std::string& layer : optional_layers)
        {
                if (supported_layers.contains(layer))
                {
                        res.insert(layer);
                }
        }

        return res;
}

std::unordered_set<std::string> make_extensions(
        const std::unordered_set<std::string>& required_extensions,
        const std::unordered_set<std::string>& optional_extensions)
{
        std::unordered_set<std::string> res;

        const std::unordered_set<std::string> supported = supported_instance_extensions();

        for (const std::string& extension : required_extensions)
        {
                if (!supported.contains(extension))
                {
                        error("Vulkan instance extension " + extension + " is not supported");
                }
                res.insert(extension);
        }

        if (!supported.contains(DEBUG))
        {
                error(std::string("Vulkan instance extension ") + DEBUG + " is not supported");
        }
        res.emplace(DEBUG);

        for (const std::string& extension : optional_extensions)
        {
                if (supported.contains(extension))
                {
                        res.insert(extension);
                }
        }

        return res;
}

std::unordered_map<std::uint32_t, std::uint32_t> compute_queue_count(
        const std::vector<std::tuple<std::uint32_t, std::uint32_t>>& family_index_and_count,
        const std::vector<VkQueueFamilyProperties>& queue_family_properties)
{
        std::unordered_map<std::uint32_t, std::uint32_t> queues;
        for (const auto& [index, count] : family_index_and_count)
        {
                if (index != NO_FAMILY_INDEX)
                {
                        queues[index] = std::min(queues[index] + count, queue_family_properties[index].queueCount);
                }
        }
        return queues;
}

template <typename T>
void check_family_indices(const CommandPool& pool, const T& queues)
{
        if (!std::all_of(
                    queues.begin(), queues.end(),
                    [&](const Queue& queue)
                    {
                            return pool.family_index() == queue.family_index();
                    }))
        {
                error("Error pool and queue family indices");
        }
}

template <std::size_t SIZE>
void set_queues(
        const Device& device,
        const std::string_view& name,
        const std::uint32_t family_index,
        std::array<Queue, SIZE>* const queues,
        std::unordered_map<std::uint32_t, std::uint32_t>* const queue_count,
        std::string* const description)
{
        if (!description->empty())
        {
                *description += '\n';
        }

        *description += name;
        *description += " queues, family index = " + to_string(family_index);

        for (std::size_t i = 0; i < queues->size(); ++i)
        {
                std::uint32_t& device_queue = (*queue_count)[family_index];
                if (device_queue >= device.queue_count(family_index))
                {
                        device_queue = 0;
                }
                (*queues)[i] = device.queue(family_index, device_queue);
                *description += "\n  queue = " + to_string(i) + ", device queue = " + to_string(device_queue);
                ++device_queue;
        }
}
}

VulkanInstance::VulkanInstance(
        const InstanceFunctionality& instance_functionality,
        const DeviceFunctionality& device_functionality,
        const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        : layers_(make_layers(instance_functionality.required_layers, instance_functionality.optional_layers)),
          extensions_(make_extensions(
                  instance_functionality.required_extensions,
                  instance_functionality.optional_extensions)),
          instance_(create_instance(layers_, extensions_)),
          instance_extension_functions_(
                  create_surface ? std::optional<InstanceExtensionFunctions>(instance_) : std::nullopt),
          messenger_(
                  extensions_.contains(DEBUG) ? std::make_optional(create_debug_utils_messenger(instance_))
                                              : std::nullopt),
          surface_(create_surface ? std::optional(handle::SurfaceKHR(instance_, create_surface)) : std::nullopt),
          physical_device_(find_physical_device(
                  instance_,
                  (create_surface ? static_cast<VkSurfaceKHR>(*surface_) : VK_NULL_HANDLE),
                  device_functionality)),
          graphics_compute_family_index_(
                  physical_device_.family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, {0})),
          compute_family_index_(
                  physical_device_.family_index(VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, {VK_QUEUE_COMPUTE_BIT})),
          transfer_family_index_(physical_device_.family_index(
                  VK_QUEUE_TRANSFER_BIT,
                  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
                  // All commands that are allowed on a queue that supports
                  // transfer operations are also allowed on a queue that
                  // supports either graphics or compute operations
                  {VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT})),
          presentation_family_index_(create_surface ? physical_device_.presentation_family_index() : NO_FAMILY_INDEX),
          device_(&physical_device_,
                  compute_queue_count(
                          {{graphics_compute_family_index_, GRAPHICS_COMPUTE_QUEUE_COUNT},
                           {compute_family_index_, COMPUTE_QUEUE_COUNT},
                           {transfer_family_index_, TRANSFER_QUEUE_COUNT},
                           {presentation_family_index_, PRESENTATION_QUEUE_COUNT}},
                          physical_device_.queue_families()),
                  device_functionality),
          device_extension_functions_(create_surface ? std::optional<DeviceExtensionFunctions>(device_) : std::nullopt),
          graphics_compute_command_pool_(create_command_pool(device_, graphics_compute_family_index_)),
          compute_command_pool_(create_command_pool(device_, compute_family_index_)),
          transfer_command_pool_(create_transient_command_pool(device_, transfer_family_index_))
{
        std::string description;
        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;

        set_queues(
                device_, "graphics compute", graphics_compute_family_index_, &graphics_compute_queues_, &queue_count,
                &description);

        set_queues(device_, "compute", compute_family_index_, &compute_queues_, &queue_count, &description);

        set_queues(device_, "transfer", transfer_family_index_, &transfer_queues_, &queue_count, &description);

        if (create_surface)
        {
                set_queues(
                        device_, "presentation", presentation_family_index_, &presentation_queues_, &queue_count,
                        &description);
        }

        LOG(description);

        check_family_indices(graphics_compute_command_pool_, graphics_compute_queues_);
        check_family_indices(compute_command_pool_, compute_queues_);
        check_family_indices(transfer_command_pool_, transfer_queues_);
}

VulkanInstance::~VulkanInstance()
{
        device_wait_idle_noexcept("the Vulkan instance destructor");
}

void VulkanInstance::device_wait_idle() const
{
        ASSERT(device_ != VK_NULL_HANDLE);

        VULKAN_CHECK(vkDeviceWaitIdle(device_));
}

void VulkanInstance::device_wait_idle_noexcept(const char* const msg) const noexcept
{
        try
        {
                try
                {
                        device_wait_idle();
                }
                catch (const std::exception& e)
                {
                        if (!msg)
                        {
                                error_fatal("No message for the device wait idle function");
                        }
                        std::string s = "Device wait idle exception in " + std::string(msg) + ": " + e.what();
                        LOG(s);
                }
                catch (...)
                {
                        if (!msg)
                        {
                                error_fatal("No message for the device wait idle function");
                        }
                        std::string s = "Device wait idle unknown exception in " + std::string(msg);
                        LOG(s);
                }
        }
        catch (...)
        {
                error_fatal("Exception in the device wait idle exception handlers");
        }
}
}
