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
constexpr std::uint32_t NO_FAMILY_INDEX = Limits<std::uint32_t>::max();

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

std::vector<std::string> merge_required_device_extensions(
        const bool with_swapchain,
        const std::vector<std::string>& required_device_extensions)
{
        if (with_swapchain)
        {
                std::vector<std::string> extensions = required_device_extensions;
                extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                return extensions;
        }
        return required_device_extensions;
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
}

VulkanInstance::VulkanInstance(
        const std::vector<std::string>& required_instance_extensions,
        const std::vector<std::string>& required_device_extensions,
        const DeviceFeatures& required_device_features,
        const DeviceFeatures& optional_device_features,
        const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        : instance_(create_instance(required_instance_extensions)),
          callback_(
                  instance_.layers_enabled() ? std::make_optional(create_debug_report_callback(instance_))
                                             : std::nullopt),
          surface_(create_surface ? std::optional(handle::SurfaceKHR(instance_, create_surface)) : std::nullopt),
          //
          physical_device_(find_physical_device(
                  instance_,
                  //
                  (create_surface ? static_cast<VkSurfaceKHR>(*surface_) : VK_NULL_HANDLE),
                  //
                  merge_required_device_extensions(create_surface != nullptr, required_device_extensions),
                  //
                  required_device_features)),
          //
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
          //
          device_(create_device(
                  &physical_device_,
                  compute_queue_count(
                          {{graphics_compute_family_index_, GRAPHICS_COMPUTE_QUEUE_COUNT},
                           {compute_family_index_, COMPUTE_QUEUE_COUNT},
                           {transfer_family_index_, TRANSFER_QUEUE_COUNT},
                           {presentation_family_index_, PRESENTATION_QUEUE_COUNT}},
                          physical_device_.queue_families()),
                  //
                  merge_required_device_extensions(create_surface != nullptr, required_device_extensions),
                  //
                  required_device_features,
                  //
                  optional_device_features)),
          //
          graphics_compute_command_pool_(create_command_pool(device_, graphics_compute_family_index_)),
          compute_command_pool_(create_command_pool(device_, compute_family_index_)),
          transfer_command_pool_(create_transient_command_pool(device_, transfer_family_index_))
{
        std::string s;
        std::unordered_map<std::uint32_t, std::uint32_t> queue_count;
        std::uint32_t index;

        index = graphics_compute_family_index_;
        s += "Graphics compute queues, family index = " + to_string(index);
        for (std::size_t i = 0; i < graphics_compute_queues_.size(); ++i)
        {
                graphics_compute_queues_[i] = device_.queue(index, queue_count[index]);
                s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                ++queue_count[index];
                if (queue_count[index] >= physical_device_.queue_families()[index].queueCount)
                {
                        queue_count[index] = 0;
                }
        }

        s += '\n';
        index = compute_family_index_;
        s += "Compute queues, family index = " + to_string(index);
        for (std::size_t i = 0; i < compute_queues_.size(); ++i)
        {
                compute_queues_[i] = device_.queue(index, queue_count[index]);
                s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                ++queue_count[index];
                if (queue_count[index] >= physical_device_.queue_families()[index].queueCount)
                {
                        queue_count[index] = 0;
                }
        }

        s += '\n';
        index = transfer_family_index_;
        s += "Transfer queues, family index = " + to_string(index);
        for (std::size_t i = 0; i < transfer_queues_.size(); ++i)
        {
                transfer_queues_[i] = device_.queue(index, queue_count[index]);
                s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                ++queue_count[index];
                if (queue_count[index] >= physical_device_.queue_families()[index].queueCount)
                {
                        queue_count[index] = 0;
                }
        }

        if (create_surface)
        {
                s += '\n';
                index = presentation_family_index_;
                s += "Presentation queues, family index = " + to_string(index);
                for (std::size_t i = 0; i < presentation_queues_.size(); ++i)
                {
                        presentation_queues_[i] = device_.queue(index, queue_count[index]);
                        s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                        ++queue_count[index];
                        if (queue_count[index] >= physical_device_.queue_families()[index].queueCount)
                        {
                                queue_count[index] = 0;
                        }
                }
        }

        LOG(s);

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
