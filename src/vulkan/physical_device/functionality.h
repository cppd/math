/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "info.h"

#include <string>
#include <unordered_set>

namespace ns::vulkan
{
struct DeviceFunctionality final
{
        std::unordered_set<std::string> required_extensions;
        std::unordered_set<std::string> optional_extensions;
        PhysicalDeviceFeatures required_features;
        PhysicalDeviceFeatures optional_features;

        void merge(const DeviceFunctionality& functionality);
};
}
