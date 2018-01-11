/*
Copyright (C) 2018 Topological Manifold

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

#include "com/vec.h"

#include <array>

vec3 srgb_integer_to_rgb_float(unsigned char r, unsigned char g, unsigned char b);

std::array<unsigned char, 3> rgb_float_to_srgb_integer(const vec3& c);

double luminosity_rgb(const vec3& v);
