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

#include <src/color/color.h>

namespace ns::shading::test
{
void check_color_equal(const Color& directional_albedo, const Color& surface_color);
void check_color_less(const Color& directional_albedo, const Color& surface_color);
void check_color_range(const Color& directional_albedo);

Color random_non_black_color();
}
