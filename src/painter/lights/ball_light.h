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

#include "../objects.h"

#include <src/geometry/spatial/hyperplane_ball.h>
#include <src/numerical/complement.h>
#include <src/numerical/vec.h>
#include <src/sampling/pdf.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cmath>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class BallLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        geometry::HyperplaneBall<N, T> ball_;
        Color color_;
        T pdf_;
        std::array<Vector<N, T>, N - 1> vectors_;

public:
        BallLight(const Vector<N, T>& center, const Vector<N, T>& normal, const T& radius, const Color& color)
                : ball_(center, normal, radius),
                  color_(color),
                  pdf_(sampling::uniform_in_sphere_pdf<std::tuple_size_v<decltype(vectors_)>>(radius)),
                  vectors_(numerical::orthogonal_complement_of_unit_vector(ball_.normal()))
        {
                for (Vector<N, T>& v : vectors_)
                {
                        v *= radius;
                }
        }

        LightSourceSample<N, T, Color> sample(RandomEngine<T>& random_engine, const Vector<N, T>& point) const override
        {
                const Vector<N, T> sample_location =
                        ball_.center() + sampling::uniform_in_sphere(vectors_, random_engine);

                const Vector<N, T> direction = sample_location - point;
                const T distance = direction.norm();
                const Vector<N, T> l = direction / distance;

                const T cos = std::abs(dot(l, ball_.normal()));

                LightSourceSample<N, T, Color> s;
                s.l = l;
                s.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, distance);
                s.radiance = color_;
                s.distance = distance;
                return s;
        }

        LightSourceInfo<T, Color> info(const Vector<N, T>& point, const Vector<N, T>& l) const override
        {
                const Ray<N, T> ray(point, l);
                const auto intersection = ball_.intersect(ray);
                if (!intersection)
                {
                        LightSourceInfo<T, Color> info;
                        info.pdf = 0;
                        return info;
                }

                const T cos = std::abs(dot(ray.dir(), ball_.normal()));

                LightSourceInfo<T, Color> info;
                info.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, *intersection);
                info.radiance = color_;
                info.distance = *intersection;
                return info;
        }

        bool is_delta() const override
        {
                return false;
        }
};
}
