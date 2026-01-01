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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.3 The BRDF
Lambertian BRDF (9.11)
*/

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

13.10 Importance sampling
*/

#pragma once

#include <src/com/error.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_cosine.h>
#include <src/shading/objects.h>

#include <cstddef>

namespace ns::shading::lambertian
{
namespace implementation
{
template <std::size_t N, typename Color>
Color f(const Color& color)
{
        // f = color / (integrate dot(n,l) over hemisphere)

        static constexpr typename Color::DataType CONSTANT_REFLECTANCE_FACTOR =
                1 / geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;

        return CONSTANT_REFLECTANCE_FACTOR * color;
}
}

//

template <std::size_t N, typename T, typename Color>
Color f(const Color& color, const numerical::Vector<N, T>& n, const numerical::Vector<N, T>& l)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        ASSERT(n.is_unit());
        ASSERT(l.is_unit());

        if (dot(n, l) <= 0)
        {
                return Color(0);
        }

        return impl::f<N>(color);
}

template <std::size_t N, typename T>
T pdf(const numerical::Vector<N, T>& n, const numerical::Vector<N, T>& l)
{
        static_assert(N >= 3);

        ASSERT(n.is_unit());
        ASSERT(l.is_unit());

        return sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
Sample<N, T, Color> sample_f(RandomEngine& engine, const Color& color, const numerical::Vector<N, T>& n)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        ASSERT(n.is_unit());

        const numerical::Vector<N, T> l = sampling::cosine_on_hemisphere(engine, n);

        ASSERT(l.is_unit());

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return Sample<N, T, Color>::non_usable();
        }

        const T pdf = sampling::cosine_on_hemisphere_pdf<N>(n_l);
        if (pdf <= 0)
        {
                return Sample<N, T, Color>::non_usable();
        }

        return {
                .l = l,
                .pdf = pdf,
                .brdf = impl::f<N>(color),
        };
}
}
