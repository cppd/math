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

#include "surface_sample.h"

#include "normals.h"

#include "../../objects.h"

#include <src/com/error.h>
#include <src/com/random/pcg.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::painter::integrators
{
namespace
{
template <bool WITH_PDF, std::size_t N, typename T, typename Color>
using Sample = std::conditional_t<WITH_PDF, SurfaceSamplePdf<N, T, Color>, SurfaceSample<N, T, Color>>;

template <bool WITH_PDF, std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<Sample<WITH_PDF, N, T, Color>> surface_sample(
        const SurfaceIntersection<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        PCG& engine)
{
        const Vector<N, T>& n = normals.shading;

        const painter::SurfaceSample<N, T, Color> sample = surface.sample(engine, n, v);

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

        const Color beta = sample.brdf * (n_l / sample.pdf);

        if constexpr (WITH_PDF)
        {
                return {
                        {.beta = beta, .l = l, .pdf_forward = sample.pdf, .pdf_reversed = surface.pdf(n, l, v)}
                };
        }
        else
        {
                return {
                        {.beta = beta, .l = l}
                };
        }
}
}

template <std::size_t N, typename T, typename Color>
std::optional<SurfaceSamplePdf<N, T, Color>> surface_sample_with_pdf(
        const SurfaceIntersection<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        PCG& engine)
{
        return surface_sample<true>(surface, v, normals, engine);
}

template <std::size_t N, typename T, typename Color>
std::optional<SurfaceSample<N, T, Color>> surface_sample(
        const SurfaceIntersection<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        PCG& engine)
{
        return surface_sample<false>(surface, v, normals, engine);
}

#define TEMPLATE(N, T, C)                                                                                    \
        template std::optional<SurfaceSamplePdf<(N), T, C>> surface_sample_with_pdf(                         \
                const SurfaceIntersection<(N), T, C>&, const Vector<(N), T>&, const Normals<(N), T>&, PCG&); \
        template std::optional<SurfaceSample<(N), T, C>> surface_sample(                                     \
                const SurfaceIntersection<(N), T, C>&, const Vector<(N), T>&, const Normals<(N), T>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
