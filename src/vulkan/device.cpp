/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "device.h"

#include "error.h"
#include "objects.h"

#include "device/create.h"
#include "device/queues.h"
#include "physical_device/features.h"
#include "physical_device/functionality.h"
#include "physical_device/info.h"
#include "physical_device/physical_device.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <exception>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ns::vulkan
{
Device::Device(
        const physical_device::PhysicalDevice* const physical_device,
        const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
        const physical_device::DeviceFunctionality& functionality)
        : physical_device_(physical_device),
          features_(make_features(
                  functionality.required_features,
                  functionality.optional_features,
                  physical_device_->features())),
          extensions_(make_extensions(
                  features_,
                  functionality.required_extensions,
                  functionality.optional_extensions,
                  physical_device_->extensions())),
          device_(device::create_device(physical_device_, queue_families, extensions_, features_)),
          queues_(device::find_queues(device_, queue_families))
{
}

Device::~Device()
{
        wait_idle_noexcept("device destructor");
}

VkPhysicalDevice Device::physical_device() const
{
        return physical_device_->device();
}

const physical_device::Properties& Device::properties() const
{
        return physical_device_->properties();
}

const std::unordered_set<std::string>& Device::extensions() const
{
        return extensions_;
}

const physical_device::Features& Device::features() const
{
        return features_;
}

Queue Device::queue(const std::uint32_t family_index, const std::uint32_t queue_index) const
{
        const auto iter = queues_.find(family_index);

        if (iter == queues_.cend())
        {
                error("Queue family index " + to_string(family_index) + " not found");
        }

        if (queue_index >= iter->second.size())
        {
                error("Queue " + to_string(queue_index) + " not found");
        }

        return {family_index, iter->second[queue_index]};
}

std::uint32_t Device::queue_count(const std::uint32_t family_index) const
{
        const auto iter = queues_.find(family_index);

        if (iter == queues_.cend())
        {
                error("Queue family index " + to_string(family_index) + " not found");
        }

        return iter->second.size();
}

void Device::wait_idle() const
{
        if (device_ != VK_NULL_HANDLE)
        {
                VULKAN_CHECK(vkDeviceWaitIdle(device_));
        }
}

void Device::wait_idle_noexcept(const char* const msg) const noexcept
{
        try
        {
                try
                {
                        wait_idle();
                }
                catch (const std::exception& e)
                {
                        if (!msg)
                        {
                                error_fatal("No message for the device wait idle function");
                        }
                        LOG(std::string("Device wait idle error in ") + msg + ": " + e.what());
                }
                catch (...)
                {
                        if (!msg)
                        {
                                error_fatal("No message for the device wait idle function");
                        }
                        LOG(std::string("Device wait idle unknown error in ") + msg);
                }
        }
        catch (...)
        {
                error_fatal("Error in the device wait idle exception handlers");
        }
}
}
