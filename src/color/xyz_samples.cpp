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

#include "xyz_samples.h"

#include "xyz.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <cmath>

namespace ns::color
{
namespace
{
using ComputeType = double;

constexpr XYZ VERSION = XYZ_31;

constexpr int MIN_WAVELENGTH = 380;
constexpr int MAX_WAVELENGTH = 780;

constexpr int MIN_SAMPLE_COUNT = 1;
constexpr int MAX_SAMPLE_COUNT = 1'000;

std::vector<float> create_samples(
        ComputeType(integrate)(ComputeType, ComputeType),
        const ComputeType from,
        const ComputeType to,
        const int count)
{
        if (!(from < to))
        {
                error("The starting wavelength (" + to_string(from) + ") must be less than the ending wavelength ("
                      + to_string(to) + ")");
        }
        if (!(from >= MIN_WAVELENGTH && to <= MAX_WAVELENGTH))
        {
                error("Starting and ending wavelengths [" + to_string(from) + ", " + to_string(to)
                      + "] must be in the range [" + to_string(MIN_WAVELENGTH) + ", " + to_string(MAX_WAVELENGTH)
                      + "]");
        }
        if (!(count >= MIN_SAMPLE_COUNT && count <= MAX_SAMPLE_COUNT))
        {
                error("Sample count " + to_string(count) + " must be in the range [" + to_string(MIN_SAMPLE_COUNT)
                      + ", " + to_string(MAX_SAMPLE_COUNT) + "]");
        }

        static const ComputeType y_integral = cie_y_integral<VERSION, ComputeType>(MIN_WAVELENGTH, MAX_WAVELENGTH);

        std::vector<float> samples;
        samples.reserve(count);

        ComputeType wave_1 = from;
        for (int i = 1; i <= count; ++i)
        {
                ComputeType wave_2 = std::lerp(from, to, static_cast<ComputeType>(i) / count);
                ASSERT(wave_1 < wave_2 && wave_1 >= from && wave_2 <= to);
                ComputeType v = integrate(wave_1, wave_2);
                v /= y_integral;
                samples.push_back(v);
                wave_1 = wave_2;
        }

        return samples;
}
}

std::vector<float> cie_x_samples(int from, int to, int count)
{
        return create_samples(cie_x_integral<VERSION, ComputeType>, from, to, count);
}

std::vector<float> cie_y_samples(int from, int to, int count)
{
        return create_samples(cie_y_integral<VERSION, ComputeType>, from, to, count);
}

std::vector<float> cie_z_samples(int from, int to, int count)
{
        return create_samples(cie_z_integral<VERSION, ComputeType>, from, to, count);
}
}
