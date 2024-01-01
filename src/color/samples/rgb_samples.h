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

namespace ns::color
{
inline constexpr int RGB_SAMPLES_MIN_WAVELENGTH = 380;
inline constexpr int RGB_SAMPLES_MAX_WAVELENGTH = 720;

std::vector<double> rgb_reflectance_white_samples(int from, int to, int count);
std::vector<double> rgb_reflectance_cyan_samples(int from, int to, int count);
std::vector<double> rgb_reflectance_magenta_samples(int from, int to, int count);
std::vector<double> rgb_reflectance_yellow_samples(int from, int to, int count);
std::vector<double> rgb_reflectance_red_samples(int from, int to, int count);
std::vector<double> rgb_reflectance_green_samples(int from, int to, int count);
std::vector<double> rgb_reflectance_blue_samples(int from, int to, int count);

std::vector<double> rgb_illumination_d65_white_samples(int from, int to, int count);
std::vector<double> rgb_illumination_d65_cyan_samples(int from, int to, int count);
std::vector<double> rgb_illumination_d65_magenta_samples(int from, int to, int count);
std::vector<double> rgb_illumination_d65_yellow_samples(int from, int to, int count);
std::vector<double> rgb_illumination_d65_red_samples(int from, int to, int count);
std::vector<double> rgb_illumination_d65_green_samples(int from, int to, int count);
std::vector<double> rgb_illumination_d65_blue_samples(int from, int to, int count);
}
