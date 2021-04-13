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

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>

#include <tuple>

namespace ns::painter
{
template <std::size_t N, typename T>
class Lambertian
{
        static_assert(N >= 3);

        static constexpr T CONSTANT_REFLECTANCE_FACTOR =
                T(1) / geometry::sphere_integrate_cosine_factor_over_hemisphere(N);

public:
        static Color shade(const Color& color, const Vector<N, T>& n, const Vector<N, T>& l)
        {
                static constexpr Color BLACK(0);

                ASSERT(n.is_unit());
                ASSERT(l.is_unit());

                T n_l = dot(n, l);
                if (n_l <= 0)
                {
                        return BLACK;
                }

                // f = color / (integrate dot(n,l) over hemisphere)
                // s = f * cos(n,l)
                // s = color / (integrate cos(n,l) over hemisphere) * cos(n,l)
                return CONSTANT_REFLECTANCE_FACTOR * n_l * color;
        }

        template <typename RandomEngine>
        static std::tuple<Vector<N, T>, Color> sample_shade(
                RandomEngine& random_engine,
                const Color& color,
                const Vector<N, T>& n)
        {
                static constexpr std::tuple<Vector<N, T>, Color> BLACK(Vector<N, T>(0), Color(0));

                ASSERT(n.is_unit());

                Vector<N, T> l = sampling::cosine_on_hemisphere(random_engine, n);

                ASSERT(l.is_unit());

                T n_l = dot(n, l);
                if (n_l <= 0)
                {
                        return BLACK;
                }

                // f = color / (integrate cos(n,l) over hemisphere)
                // pdf = cos(n,l) / (integrate cos(n,l) over hemisphere)
                // s = f / pdf * cos(n,l)
                // s = color / (integrate cos(n,l) over hemisphere) /
                //     (cos(n,l) / (integrate cos(n,l) over hemisphere)) * cos(n,l)
                // s = color
                return {l, color};
        }
};
}
