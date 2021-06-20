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

#include <vector>

namespace ns::color
{
constexpr int DAYLIGHT_SAMPLES_MIN_WAVELENGTH = 300;
constexpr int DAYLIGHT_SAMPLES_MAX_WAVELENGTH = 830;

constexpr double DAYLIGHT_SAMPLES_MIN_CCT = 4000;
constexpr double DAYLIGHT_SAMPLES_MAX_CCT = 25000;

std::vector<double> d65_samples(int from, int to, int count);
std::vector<double> daylight_samples(double cct, int from, int to, int count);
}
