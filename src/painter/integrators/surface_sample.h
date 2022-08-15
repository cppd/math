/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "normals.h"

#include "../objects.h"

#include <src/com/error.h>

namespace ns::painter::integrators
{
template <std::size_t N, typename T, typename Color>
struct Sample final
{
        Color beta;
        Vector<N, T> l;
};

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Sample<N, T, Color>> surface_sample(
        const SurfaceIntersection<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        const Vector<N, T>& n = normals.shading;

        const SurfaceSample<N, T, Color> sample = surface.sample(engine, n, v);

        if (sample.pdf <= 0 || sample.brdf.is_black())
        {
                return {};
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        if (dot(l, normals.geometric) <= 0)
        {
                return {};
        }

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return {};
        }

        return Sample<N, T, Color>{.beta = sample.brdf * (n_l / sample.pdf), .l = l};
}

template <std::size_t N, typename T, typename Color>
struct SampleBD final
{
        Color beta;
        Vector<N, T> l;
        T pdf_forward;
        T pdf_reversed;
};

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<SampleBD<N, T, Color>> surface_sample_bd(
        const SurfaceIntersection<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        const Vector<N, T>& n = normals.shading;

        const SurfaceSample<N, T, Color> sample = surface.sample(engine, n, v);

        if (sample.pdf <= 0 || sample.brdf.is_black())
        {
                return {};
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        if (dot(l, normals.geometric) <= 0)
        {
                return {};
        }

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return {};
        }

        const T pdf_reversed = surface.pdf(n, l, v);

        return SampleBD<N, T, Color>{
                .beta = sample.brdf * (n_l / sample.pdf),
                .l = l,
                .pdf_forward = sample.pdf,
                .pdf_reversed = pdf_reversed};
}
}
