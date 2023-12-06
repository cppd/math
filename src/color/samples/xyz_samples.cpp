/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "xyz_functions.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <cmath>
#include <vector>

namespace ns::color
{
namespace
{
constexpr int MIN_SAMPLE_COUNT = 1;
constexpr int MAX_SAMPLE_COUNT = 1'000'000;

using ComputeType = double;

enum class Function
{
        X,
        Y,
        Z
};

template <XYZ TYPE, Function F>
ComputeType integrate(const ComputeType& from, const ComputeType& to)
{
        static_assert(TYPE == XYZ_31 || TYPE == XYZ_64);
        static_assert(F == Function::X || F == Function::Y || F == Function::Z);

        switch (F)
        {
        case Function::X:
                return TYPE == XYZ_31 ? cie_x_31_integral(from, to) : cie_x_64_integral(from, to);
        case Function::Y:
                return TYPE == XYZ_31 ? cie_y_31_integral(from, to) : cie_y_64_integral(from, to);
        case Function::Z:
                return TYPE == XYZ_31 ? cie_z_31_integral(from, to) : cie_z_64_integral(from, to);
        }
}

template <XYZ TYPE, Function F>
std::vector<double> create_samples(const ComputeType& from, const ComputeType& to, const int count)
{
        constexpr ComputeType MIN = XYZ_SAMPLES_MIN_WAVELENGTH;
        constexpr ComputeType MAX = XYZ_SAMPLES_MAX_WAVELENGTH;

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

        if (!(count >= MIN_SAMPLE_COUNT && count <= MAX_SAMPLE_COUNT))
        {
                error("Sample count " + to_string(count) + " must be in the range [" + to_string(MIN_SAMPLE_COUNT)
                      + ", " + to_string(MAX_SAMPLE_COUNT) + "]");
        }

        static const ComputeType y_integral = integrate<TYPE, Function::Y>(MIN, MAX);

        std::vector<double> samples;
        samples.reserve(count);

        const ComputeType count_ct = count;
        ComputeType wave_1 = from;
        for (int i = 1; i <= count; ++i)
        {
                const ComputeType wave_2 = std::lerp(from, to, i / count_ct);
                ASSERT(wave_1 < wave_2 && wave_1 >= from && wave_2 <= to);
                const ComputeType v = integrate<TYPE, F>(wave_1, wave_2);
                samples.push_back(v / y_integral);
                wave_1 = wave_2;
        }

        return samples;
}
}

template <XYZ TYPE>
std::vector<double> cie_x_samples(const int from, const int to, const int count)
{
        return create_samples<TYPE, Function::X>(from, to, count);
}

template <XYZ TYPE>
std::vector<double> cie_y_samples(const int from, const int to, const int count)
{
        return create_samples<TYPE, Function::Y>(from, to, count);
}

template <XYZ TYPE>
std::vector<double> cie_z_samples(const int from, const int to, const int count)
{
        return create_samples<TYPE, Function::Z>(from, to, count);
}

template std::vector<double> cie_x_samples<XYZ_31>(int, int, int);
template std::vector<double> cie_y_samples<XYZ_31>(int, int, int);
template std::vector<double> cie_z_samples<XYZ_31>(int, int, int);

template std::vector<double> cie_x_samples<XYZ_64>(int, int, int);
template std::vector<double> cie_y_samples<XYZ_64>(int, int, int);
template std::vector<double> cie_z_samples<XYZ_64>(int, int, int);
}
