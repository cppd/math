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

#include <src/numerical/vector.h>

#include <cstddef>
#include <span>

namespace ns::image
{
void blend_alpha(ColorFormat* color_format, const std::span<std::byte>& bytes, Vector<3, float> rgb);

void set_alpha(ColorFormat color_format, const std::span<std::byte>& bytes, float alpha);

template <std::size_t N>
[[nodiscard]] Image<N> add_alpha(const Image<N>& image, float alpha);

template <std::size_t N>
[[nodiscard]] Image<N> delete_alpha(const Image<N>& image);
}
