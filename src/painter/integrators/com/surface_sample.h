/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/random/pcg.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>

#include <cstddef>
#include <optional>

namespace ns::painter::integrators::com
{
template <std::size_t N, typename T, typename Color>
struct SurfaceSamplePdf final
{
        Color beta;
        numerical::Vector<N, T> l;
        T pdf_forward;
        T pdf_reversed;
};

template <std::size_t N, typename T, typename Color>
struct SurfaceSample final
{
        Color beta;
        numerical::Vector<N, T> l;
};

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<SurfaceSamplePdf<N, T, Color>> surface_sample_with_pdf(
        const SurfaceIntersection<N, T, Color>& surface,
        const numerical::Vector<N, T>& v,
        const Normals<N, T>& normals,
        PCG& engine);

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<SurfaceSample<N, T, Color>> surface_sample(
        const SurfaceIntersection<N, T, Color>& surface,
        const numerical::Vector<N, T>& v,
        const Normals<N, T>& normals,
        PCG& engine);
}
