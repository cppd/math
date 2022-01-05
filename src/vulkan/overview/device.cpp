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

#include "device.h"

#include <sstream>

namespace ns::vulkan
{
std::string device_conformance_version(const PhysicalDevice& device)
{
        const VkConformanceVersion version = device.properties().properties_12.conformanceVersion;
        std::ostringstream oss;
        oss << static_cast<int>(version.major);
        oss << "." << static_cast<int>(version.minor);
        oss << "." << static_cast<int>(version.subminor);
        oss << "." << static_cast<int>(version.patch);
        return oss.str();
}

std::string device_name(const PhysicalDevice& device)
{
        return static_cast<const char*>(device.properties().properties_10.deviceName);
}

std::string device_driver_name(const PhysicalDevice& device)
{
        return static_cast<const char*>(device.properties().properties_12.driverName);
}

std::string device_driver_info(const PhysicalDevice& device)
{
        return static_cast<const char*>(device.properties().properties_12.driverInfo);
}
}
