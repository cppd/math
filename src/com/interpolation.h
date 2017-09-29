/*
Copyright (C) 2017 Topological Manifold

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

#include "math.h"

template <typename T, typename F>
constexpr T interpolation(T c00, T c10, T c01, T c11, F x, F y)
{
        T y0 = interpolation(c00, c10, x);
        T y1 = interpolation(c01, c11, x);
        T r = interpolation(y0, y1, y);
        return r;
}

template <typename T, typename F>
constexpr T interpolation(T c000, T c100, T c010, T c110, T c001, T c101, T c011, T c111, F x, F y, F z)
{
        T y0z0 = interpolation(c000, c100, x);
        T y1z0 = interpolation(c010, c110, x);
        T y0z1 = interpolation(c001, c101, x);
        T y1z1 = interpolation(c011, c111, x);
        T z0 = interpolation(y0z0, y1z0, y);
        T z1 = interpolation(y0z1, y1z1, y);
        T r = interpolation(z0, z1, z);
        return r;
}

template <typename T, typename F>
constexpr T interpolation(T c0000, T c1000, T c0100, T c1100, T c0010, T c1010, T c0110, T c1110, T c0001, T c1001, T c0101,
                          T c1101, T c0011, T c1011, T c0111, T c1111, F x, F y, F z, F w)
{
        T y0z0w0 = interpolation(c0000, c1000, x);
        T y1z0w0 = interpolation(c0100, c1100, x);
        T y0z1w0 = interpolation(c0010, c1010, x);
        T y1z1w0 = interpolation(c0110, c1110, x);
        T y0z0w1 = interpolation(c0001, c1001, x);
        T y1z0w1 = interpolation(c0101, c1101, x);
        T y0z1w1 = interpolation(c0011, c1011, x);
        T y1z1w1 = interpolation(c0111, c1111, x);
        T z0w0 = interpolation(y0z0w0, y1z0w0, y);
        T z1w0 = interpolation(y0z1w0, y1z1w0, y);
        T z0w1 = interpolation(y0z0w1, y1z0w1, y);
        T z1w1 = interpolation(y0z1w1, y1z1w1, y);
        T w0 = interpolation(z0w0, z1w0, z);
        T w1 = interpolation(z0w1, z1w1, z);
        T r = interpolation(w0, w1, w);
        return r;
}
