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

#include "color.h"

#include "samples/blackbody_samples.h"
#include "samples/daylight_samples.h"

#include <src/com/error.h>

namespace ns::color
{
namespace illuminants_implementation
{
template <typename F>
Spectrum create_spectrum(const F& f)
{
        const std::vector<double> samples =
                f(Spectrum::WAVELENGTH_MIN, Spectrum::WAVELENGTH_MAX, Spectrum::SAMPLE_COUNT);
        ASSERT(samples.size() == Spectrum::SAMPLE_COUNT);

        Vector<Spectrum::SAMPLE_COUNT, Spectrum::DataType> v;
        for (std::size_t i = 0; i < Spectrum::SAMPLE_COUNT; ++i)
        {
                v[i] = samples[i];
        }

        Spectrum spectrum = Spectrum::create_spectrum(std::move(v));
        return spectrum / spectrum.luminance();
}
}

template <typename ColorType>
ColorType daylight_d65()
{
        namespace impl = illuminants_implementation;

        static const Spectrum spectrum = impl::create_spectrum(
                [](int from, int to, int count)
                {
                        return daylight_d65_samples(from, to, count);
                });

        static_assert(std::is_same_v<ColorType, Color> || std::is_same_v<ColorType, Spectrum>);
        if constexpr (std::is_same_v<ColorType, Spectrum>)
        {
                return spectrum;
        }
        else
        {
                static const Color color = spectrum.to_rgb<Color>();
                return color;
        }
}

template <typename ColorType>
ColorType daylight(double cct)
{
        namespace impl = illuminants_implementation;

        Spectrum spectrum = impl::create_spectrum(
                [&](int from, int to, int count)
                {
                        return daylight_samples(cct, from, to, count);
                });

        static_assert(std::is_same_v<ColorType, Color> || std::is_same_v<ColorType, Spectrum>);
        if constexpr (std::is_same_v<ColorType, Spectrum>)
        {
                return spectrum;
        }
        else
        {
                return spectrum.to_rgb<Color>();
        }
}

template <typename ColorType>
ColorType blackbody_a()
{
        namespace impl = illuminants_implementation;

        static const Spectrum spectrum = impl::create_spectrum(
                [](int from, int to, int count)
                {
                        return blackbody_a_samples(from, to, count);
                });

        static_assert(std::is_same_v<ColorType, Color> || std::is_same_v<ColorType, Spectrum>);
        if constexpr (std::is_same_v<ColorType, Spectrum>)
        {
                return spectrum;
        }
        else
        {
                static const Color color = spectrum.to_rgb<Color>();
                return color;
        }
}

template <typename ColorType>
ColorType blackbody(double t)
{
        namespace impl = illuminants_implementation;

        Spectrum spectrum = impl::create_spectrum(
                [&](int from, int to, int count)
                {
                        return blackbody_samples(t, from, to, count);
                });

        static_assert(std::is_same_v<ColorType, Color> || std::is_same_v<ColorType, Spectrum>);
        if constexpr (std::is_same_v<ColorType, Spectrum>)
        {
                return spectrum;
        }
        else
        {
                return spectrum.to_rgb<Color>();
        }
}
}
