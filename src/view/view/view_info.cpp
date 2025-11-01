/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "view_info.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/vulkan/physical_device/info.h>
#include <src/vulkan/sample.h>
#include <src/vulkan/swapchain.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <flat_set>
#include <iterator>
#include <optional>

namespace ns::view::view
{
PixelSizes pixel_sizes(
        const double text_size_in_points,
        const double frame_size_in_millimeters,
        const std::array<double, 2>& window_size_in_mm,
        const vulkan::Swapchain& swapchain)
{
        const double ppi_x = size_to_ppi(window_size_in_mm[0], swapchain.width());
        const double ppi_y = size_to_ppi(window_size_in_mm[1], swapchain.height());
        const double ppi = 0.5 * (ppi_x + ppi_y);

        if (!(ppi > 0))
        {
                error("PPI " + to_string(ppi) + "is not positive");
        }

        PixelSizes res;
        res.ppi = ppi;
        res.frame = std::max(1, millimeters_to_pixels(frame_size_in_millimeters, ppi));
        res.text = std::max(1, points_to_pixels(text_size_in_points, ppi));
        return res;
}

VkSampleCountFlagBits sample_count_flag_preferred(
        const bool multisampling,
        const int preferred_sample_count,
        const vulkan::physical_device::Properties& properties)
{
        const int sample_count = [&]
        {
                const std::flat_set<int> sample_counts =
                        vulkan::supported_sample_counts(properties.properties_10.limits);
                const auto iter = sample_counts.lower_bound(preferred_sample_count);
                if (iter == sample_counts.cend())
                {
                        ASSERT(!sample_counts.empty());
                        return *std::prev(iter);
                }
                return *iter;
        }();

        if (multisampling && sample_count < 2)
        {
                error("At least 2 sample count is required");
        }

        return vulkan::sample_count_to_sample_count_flag(sample_count);
}

std::optional<VkSampleCountFlagBits> sample_count_flag(
        const bool multisampling,
        const int sample_count,
        const vulkan::physical_device::Properties& properties)
{
        if ((multisampling && sample_count < 2)
            || !vulkan::supported_sample_counts(properties.properties_10.limits).contains(sample_count))
        {
                return std::nullopt;
        }
        return vulkan::sample_count_to_sample_count_flag(sample_count);
}

std::flat_set<int> sample_counts(const bool multisampling, const vulkan::physical_device::Properties& properties)
{
        std::flat_set<int> counts = vulkan::supported_sample_counts(properties.properties_10.limits);
        if (multisampling)
        {
                counts.erase(counts.cbegin(), counts.lower_bound(2));
        }
        return counts;
}
}
