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

#include "illuminants.h"

#include "color.h"

#include "samples/blackbody_samples.h"
#include "samples/daylight_samples.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <vector>

namespace ns::color
{
namespace
{
static_assert(DAYLIGHT_SAMPLES_MIN_WAVELENGTH <= Spectrum::WAVELENGTH_MIN);
static_assert(DAYLIGHT_SAMPLES_MAX_WAVELENGTH >= Spectrum::WAVELENGTH_MAX);

template <typename F>
Spectrum create_spectrum(const F& f)
{
        const std::vector<double> samples =
                f(Spectrum::WAVELENGTH_MIN, Spectrum::WAVELENGTH_MAX, Spectrum::SAMPLE_COUNT);
        ASSERT(samples.size() == Spectrum::SAMPLE_COUNT);

        numerical::Vector<Spectrum::SAMPLE_COUNT, Spectrum::DataType> v;
        for (std::size_t i = 0; i < Spectrum::SAMPLE_COUNT; ++i)
        {
                v[i] = samples[i];
        }

        const Spectrum spectrum(v);
        return spectrum / spectrum.luminance();
}
}

const Spectrum& daylight_d65()
{
        static const Spectrum spectrum = create_spectrum(
                [](int from, int to, int count)
                {
                        return daylight_d65_samples(from, to, count);
                });

        return spectrum;
}

double daylight_min_cct()
{
        return DAYLIGHT_SAMPLES_MIN_CCT;
}

double daylight_max_cct()
{
        return DAYLIGHT_SAMPLES_MAX_CCT;
}

Spectrum daylight(const double cct)
{
        return create_spectrum(
                [&](const int from, const int to, const int count)
                {
                        return daylight_samples(cct, from, to, count);
                });
}

const Spectrum& blackbody_a()
{
        static const Spectrum spectrum = create_spectrum(
                [](const int from, const int to, const int count)
                {
                        return blackbody_a_samples(from, to, count);
                });

        return spectrum;
}

Spectrum blackbody(const double t)
{
        return create_spectrum(
                [&](const int from, const int to, const int count)
                {
                        return blackbody_samples(t, from, to, count);
                });
}
}
