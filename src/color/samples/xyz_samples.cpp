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
constexpr int SAMPLES_MIN_COUNT = 1;
constexpr int SAMPLES_MAX_COUNT = 1'000'000;

using ComputeType = double;

enum class F
{
        X,
        Y,
        Z
};

template <XYZ xyz, F f>
ComputeType integrate(ComputeType from, ComputeType to)
{
        static_assert(f == F::X || f == F::Y || f == F::Z);

        if constexpr (f == F::X)
        {
                return cie_x_integral<xyz, ComputeType>(from, to);
        }
        if constexpr (f == F::Y)
        {
                return cie_y_integral<xyz, ComputeType>(from, to);
        }
        if constexpr (f == F::Z)
        {
                return cie_z_integral<xyz, ComputeType>(from, to);
        }
}

template <XYZ xyz, F f>
std::vector<double> create_samples(const ComputeType from, const ComputeType to, const int count)
{
        constexpr double MIN = XYZ_SAMPLES_MIN_WAVELENGTH;
        constexpr double MAX = XYZ_SAMPLES_MAX_WAVELENGTH;

        if (!(from < to))
        {
                error("The starting wavelength (" + to_string(from) + ") must be less than the ending wavelength ("
                      + to_string(to) + ")");
        }
        if (!(from >= MIN && to <= MAX))
        {
                error("Starting and ending wavelengths [" + to_string(from) + ", " + to_string(to)
                      + "] must be in the range [" + to_string(MIN) + ", " + to_string(MAX) + "]");
        }
        if (!(count >= SAMPLES_MIN_COUNT && count <= SAMPLES_MAX_COUNT))
        {
                error("Sample count " + to_string(count) + " must be in the range [" + to_string(SAMPLES_MIN_COUNT)
                      + ", " + to_string(SAMPLES_MAX_COUNT) + "]");
        }

        static const ComputeType y_integral = cie_y_integral<xyz, ComputeType>(MIN, MAX);

        std::vector<double> samples;
        samples.reserve(count);

        ComputeType wave_1 = from;
        for (int i = 1; i <= count; ++i)
        {
                ComputeType wave_2 = std::lerp(from, to, static_cast<ComputeType>(i) / count);
                ASSERT(wave_1 < wave_2 && wave_1 >= from && wave_2 <= to);
                ComputeType v = integrate<xyz, f>(wave_1, wave_2);
                v /= y_integral;
                samples.push_back(v);
                wave_1 = wave_2;
        }

        return samples;
}
}

template <XYZ xyz>
std::vector<double> cie_x_samples(int from, int to, int count)
{
        return create_samples<xyz, F::X>(from, to, count);
}

template <XYZ xyz>
std::vector<double> cie_y_samples(int from, int to, int count)
{
        return create_samples<xyz, F::Y>(from, to, count);
}

template <XYZ xyz>
std::vector<double> cie_z_samples(int from, int to, int count)
{
        return create_samples<xyz, F::Z>(from, to, count);
}

template std::vector<double> cie_x_samples<XYZ_31>(int, int, int);
template std::vector<double> cie_y_samples<XYZ_31>(int, int, int);
template std::vector<double> cie_z_samples<XYZ_31>(int, int, int);

template std::vector<double> cie_x_samples<XYZ_64>(int, int, int);
template std::vector<double> cie_y_samples<XYZ_64>(int, int, int);
template std::vector<double> cie_z_samples<XYZ_64>(int, int, int);
}
