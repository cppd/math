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

#pragma once

#include <src/vulkan/physical_device/info.h>
#include <src/vulkan/swapchain.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <flat_set>
#include <optional>

namespace ns::view::view
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
        const vulkan::physical_device::Properties& properties);

std::optional<VkSampleCountFlagBits> sample_count_flag(
        bool multisampling,
        int sample_count,
        const vulkan::physical_device::Properties& properties);

std::flat_set<int> sample_counts(bool multisampling, const vulkan::physical_device::Properties& properties);
}
