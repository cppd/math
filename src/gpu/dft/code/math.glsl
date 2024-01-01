/*
Copyright (C) 2017-2024 Topological Manifold

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

#ifndef MATH_GLSL
#define MATH_GLSL

const float PI = 3.1415926535897932384626433832795028841971693993751;

uint bit_reverse(const uint value, const uint bit_width)
{
        return bitfieldReverse(value) >> (32 - bit_width);
}

vec2 complex_mul(const vec2 a, const vec2 b)
{
        const float x = a.x * b.x - a.y * b.y;
        const float y = a.x * b.y + a.y * b.x;
        return vec2(x, y);
}

#endif
