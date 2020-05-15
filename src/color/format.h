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

#include "color.h"

#include <src/com/span.h>

#include <cstddef>
#include <string>
#include <vector>

namespace color
{
std::string format_to_string(ColorFormat format);
unsigned pixel_size_in_bytes(ColorFormat format);
unsigned component_count(ColorFormat format);

void format_conversion(
        ColorFormat from_format,
        const Span<const std::byte>& from,
        ColorFormat to_format,
        std::vector<std::byte>* to);
}
