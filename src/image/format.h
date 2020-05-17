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

#include <string>

namespace image
{
enum class ColorFormat
{
        R8_SRGB,
        R8G8B8_SRGB,
        R8G8B8A8_SRGB,
        R16,
        R16G16B16,
        R16G16B16A16,
        R32,
        R32G32B32,
        R32G32B32A32,
};

std::string format_to_string(ColorFormat format);
unsigned format_pixel_size_in_bytes(ColorFormat format);
unsigned format_component_count(ColorFormat format);
}
