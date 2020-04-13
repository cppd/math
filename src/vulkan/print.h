/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <array>
#include <string>
#include <vulkan/vulkan.h>

namespace vulkan
{
std::string api_version_to_string(uint32_t api_version);
std::array<std::string, 2> result_to_strings(const VkResult& code);
std::string physical_device_type_to_string(VkPhysicalDeviceType type);
std::string format_to_string(VkFormat format);
std::string color_space_to_string(VkColorSpaceKHR color_space);
std::string image_type_to_string(VkImageType image_type);
}
