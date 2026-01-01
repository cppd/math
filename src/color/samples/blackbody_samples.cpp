/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "blackbody_samples.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/numerical/integrate.h>

#include <cmath>
#include <vector>

namespace ns::color::samples
{
namespace
{
constexpr int MIN_SAMPLE_COUNT = 1;
constexpr int MAX_SAMPLE_COUNT = 1'000'000;

constexpr double PLANCK_CONSTANT = 6.62607015e-34;
constexpr double BOLTZMANN_CONSTANT = 1.380649e-23;
constexpr double SPEED_OF_LIGHT = 299792458;

double plank(double l, const double t)
{
        l *= 1e-9;
        return (2 * PLANCK_CONSTANT * SPEED_OF_LIGHT * SPEED_OF_LIGHT)
               / (power<5>(l) * (std::exp((PLANCK_CONSTANT * SPEED_OF_LIGHT / BOLTZMANN_CONSTANT) / (l * t)) - 1));
}

double plank_a(double l)
{
        l *= 1e-9;
        return (2 * PLANCK_CONSTANT * SPEED_OF_LIGHT * SPEED_OF_LIGHT)
               / (power<5>(l) * (std::exp((0.01435 / 2848) / l) - 1));
}

template <typename F>
std::vector<double> create_samples(const int from, const int to, const int count, const F& f)
{
        if (!(from < to))
        {
                error("The starting wavelength (" + to_string(from) + ") must be less than the ending wavelength ("
                      + to_string(to) + ")");
        }

        if (!(from > 0))
        {
                error("Starting wavelength " + to_string(from) + " must be positive");
        }

        if (!(count >= MIN_SAMPLE_COUNT && count <= MAX_SAMPLE_COUNT))
        {
                error("Sample count " + to_string(count) + " must be in the range [" + to_string(MIN_SAMPLE_COUNT)
                      + ", " + to_string(MAX_SAMPLE_COUNT) + "]");
        }

        std::vector<double> samples;
        samples.reserve(count);

        constexpr int INTEGRATE_COUNT = 100;
        const double count_d = count;
        double wave_1 = from;
        for (int i = 1; i <= count; ++i)
        {
                const double wave_2 = std::lerp(from, to, i / count_d);
                ASSERT(wave_1 < wave_2 && wave_1 >= from && wave_2 <= to);
                const double v = numerical::integrate(f, wave_1, wave_2, INTEGRATE_COUNT);
                samples.push_back(v / (wave_2 - wave_1));
                wave_1 = wave_2;
        }

        return samples;
}
}

std::vector<double> blackbody_a_samples(const int from, const int to, const int count)
{
        return create_samples(
                from, to, count,
                [](double l)
                {
                        return plank_a(l);
                });
}

std::vector<double> blackbody_samples(const double t, const int from, const int to, const int count)
{
        if (!(t > 0))
        {
                error("Color temperature " + to_string(t) + " must be positive");
        }

        return create_samples(
                from, to, count,
                [t](double l)
                {
                        return plank(l, t);
                });
}
}
