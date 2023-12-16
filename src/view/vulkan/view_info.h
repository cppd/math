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

#include <src/vulkan/physical_device/info.h>
#include <src/vulkan/swapchain.h>

#include <array>
#include <optional>
#include <set>

namespace ns::view
{
struct PixelSizes final
{
        double ppi;
        unsigned frame;
        unsigned text;
};

PixelSizes pixel_sizes(
        double text_size_in_points,
        double frame_size_in_millimeters,
        const std::array<double, 2>& window_size_in_mm,
        const vulkan::Swapchain& swapchain);

VkSampleCountFlagBits sample_count_flag_preferred(
        bool multisampling,
        int preferred_sample_count,
        const vulkan::PhysicalDeviceProperties& properties);

std::optional<VkSampleCountFlagBits> sample_count_flag(
        bool multisampling,
        int sample_count,
        const vulkan::PhysicalDeviceProperties& properties);

std::set<int> sample_counts(bool multisampling, const vulkan::PhysicalDeviceProperties& properties);
}
