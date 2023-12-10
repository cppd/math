/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "format.h"
#include "image.h"

#include <cstddef>
#include <span>

namespace ns::image
{
void make_grayscale(ColorFormat color_format, const std::span<std::byte>& bytes);

template <std::size_t N>
[[nodiscard]] Image<N> convert_to_r_component_format(const Image<N>& image);
}
