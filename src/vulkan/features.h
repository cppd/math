/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "objects.h"

#include <vector>

namespace ns::vulkan
{
DeviceFeatures make_features(
        const std::vector<DeviceFeatures>& required_features,
        const std::vector<DeviceFeatures>& optional_features,
        const DeviceFeatures& supported_features);

[[nodiscard]] bool check_features(
        const std::vector<DeviceFeatures>& required_features,
        const DeviceFeatures& supported_features);
}
