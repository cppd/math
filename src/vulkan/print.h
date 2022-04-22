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

#pragma once

#include "print/color_space.h"
#include "print/flags.h"
#include "print/format.h"
#include "print/formats.h"
#include "print/image_layout.h"
#include "print/image_type.h"
#include "print/physical_device_type.h"
#include "print/point_clipping_behavior.h"
#include "print/present_mode.h"
#include "print/result.h"
#include "print/shader_float_controls_independence.h"

#include <algorithm>
#include <string>

namespace ns::vulkan
{
std::string api_version_to_string(std::uint32_t api_version);

template <typename T>
std::string strings_to_sorted_string(const T& strings)
{
        if (std::empty(strings))
        {
                return {};
        }

        if (std::size(strings) == 1)
        {
                return strings[0];
        }

        std::vector<std::string_view> data(std::begin(strings), std::end(strings));

        std::sort(data.begin(), data.end());

        std::string s;
        s += data[0];
        for (std::size_t i = 1; i < data.size(); ++i)
        {
                s += ", ";
                s += data[i];
        }
        return s;
}
}
