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

#pragma once

#include <vulkan/vulkan_core.h>

#include <string>
#include <vector>

namespace ns::vulkan::strings
{
std::vector<std::string> sample_counts_to_strings(VkSampleCountFlags flags);
std::vector<std::string> resolve_modes_to_strings(VkResolveModeFlags flags);
std::vector<std::string> shader_stages_to_strings(VkShaderStageFlags flags);
std::vector<std::string> subgroup_features_to_strings(VkSubgroupFeatureFlags flags);
std::vector<std::string> queues_to_strings(VkQueueFlags flags);
}
