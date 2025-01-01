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

#include "info.h"

#include <string>
#include <vector>

namespace ns::vulkan::physical_device
{
void add_features(Features* dst, const Features& src);

Features make_features(const Features& required, const Features& optional, const Features& supported);

[[nodiscard]] bool check_features(const Features& required, const Features& supported);

std::vector<std::string> features_to_strings(const Features& features, bool enabled);

template <typename Features>
bool any_feature_enabled(const Features& features);
}
