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

#include "../objects.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class LambertianBRDF
{
        static_assert(N >= 3);

        static Color f(const Color& color)
        {
                // f = color / (integrate dot(n,l) over hemisphere)

                static constexpr T CONSTANT_REFLECTANCE_FACTOR =
                        T(1) / geometry::sphere_integrate_cosine_factor_over_hemisphere(N);

                return CONSTANT_REFLECTANCE_FACTOR * color;
        }

public:
        static Color f(const Color& color, const Vector<N, T>& n, const Vector<N, T>& l)
        {
                static constexpr Color BLACK(0);

                ASSERT(n.is_unit());
                ASSERT(l.is_unit());

                if (dot(n, l) <= 0)
                {
                        return BLACK;
                }

                return f(color);
        }

        template <typename RandomEngine>
        static BrdfSample<N, T> sample_f(RandomEngine& random_engine, const Color& color, const Vector<N, T>& n)
        {
                static constexpr BrdfSample<N, T> BLACK(Vector<N, T>(0), 0, Color(0));

                ASSERT(n.is_unit());

                Vector<N, T> l = sampling::cosine_on_hemisphere(random_engine, n);

                ASSERT(l.is_unit());

                T n_l = dot(n, l);
                if (n_l <= 0)
                {
                        return BLACK;
                }

                T pdf = sampling::cosine_on_hemisphere_pdf<N>(n_l);
                if (pdf <= 0)
                {
                        return BLACK;
                }

                return {l, pdf, f(color)};
        }
};
}
