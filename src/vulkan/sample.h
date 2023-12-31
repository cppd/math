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

#include <vulkan/vulkan_core.h>

#include <set>

namespace ns::vulkan
{
std::set<int> supported_sample_counts(const VkPhysicalDeviceLimits& limits);

VkSampleCountFlagBits sample_count_to_sample_count_flag(int sample_count);
int sample_count_flag_to_sample_count(VkSampleCountFlagBits sample_count);
}
