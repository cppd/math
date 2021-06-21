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

#include "blackbody_samples.h"

#include <src/com/error.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/numerical/integrate.h>

#include <cmath>

namespace ns::color
{
namespace
{
double plank(double l, double t)
{
        // The Planck constant
        constexpr double h = 6.62607015e-34;
        // The Boltzmann constant
        constexpr double kb = 1.380649e-23;
        // The speed of light
        constexpr double c = 299792458;

        l *= 1e-9;

        return (2 * h * c * c) / (power<5>(l) * (std::exp((h * c) / (l * kb * t)) - 1));
}
}

std::vector<double> blackbody_samples(double t, int from, int to, int count)
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
        if (!(count >= BLACKBODY_SAMPLES_MIN_COUNT && count <= BLACKBODY_SAMPLES_MAX_COUNT))
        {
                error("Sample count " + to_string(count) + " must be in the range ["
                      + to_string(BLACKBODY_SAMPLES_MIN_COUNT) + ", " + to_string(BLACKBODY_SAMPLES_MAX_COUNT) + "]");
        }

        const auto f = [t](double l)
        {
                return plank(l, t);
        };

        std::vector<double> samples;
        samples.reserve(count);

        constexpr int INTEGRATE_COUNT = 100;
        double wave_1 = from;
        for (int i = 1; i <= count; ++i)
        {
                double wave_2 = std::lerp(from, to, static_cast<double>(i) / count);
                ASSERT(wave_1 < wave_2 && wave_1 >= from && wave_2 <= to);
                double v = numerical::integrate(f, wave_1, wave_2, INTEGRATE_COUNT);
                samples.push_back(v / (wave_2 - wave_1));
                wave_1 = wave_2;
        }

        return samples;
}
}
