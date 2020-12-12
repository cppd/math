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

namespace colors
{
inline constexpr Color BLACK = Color(Srgb8(0, 0, 0));
inline constexpr Color BLUE = Color(Srgb8(0, 0, 255));
inline constexpr Color CYAN = Color(Srgb8(0, 255, 255));
inline constexpr Color GREEN = Color(Srgb8(0, 255, 0));
inline constexpr Color MAGENTA = Color(Srgb8(255, 0, 255));
inline constexpr Color RED = Color(Srgb8(255, 0, 0));
inline constexpr Color WHITE = Color(Srgb8(255, 255, 255));
inline constexpr Color YELLOW = Color(Srgb8(255, 255, 0));
}
