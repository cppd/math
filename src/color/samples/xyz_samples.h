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

#pragma once

#include <vector>

namespace ns::color::samples
{
enum XYZ
{
        XYZ_31,
        XYZ_64
};

inline constexpr int XYZ_SAMPLES_MIN_WAVELENGTH = 380;
inline constexpr int XYZ_SAMPLES_MAX_WAVELENGTH = 780;

template <XYZ TYPE>
std::vector<double> cie_x_samples(int from, int to, int count);

template <XYZ TYPE>
std::vector<double> cie_y_samples(int from, int to, int count);

template <XYZ TYPE>
std::vector<double> cie_z_samples(int from, int to, int count);
}
