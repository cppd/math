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

#include "print/color_space.h"
#include "print/format.h"
#include "print/image_layout.h"
#include "print/image_type.h"
#include "print/physical_device_type.h"
#include "print/result.h"

#include <string>

namespace ns::vulkan
{
std::string api_version_to_string(std::uint32_t api_version);
}
