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

#include <src/geometry/spatial/hyperplane_parallelotope.h>
#include <src/numerical/vec.h>
#include <src/sampling/parallelotope_uniform.h>
#include <src/sampling/pdf.h>

#include <array>
#include <cmath>
#include <random>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class ParallelotopeLight final : public LightSource<N, T, Color>
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        geometry::HyperplaneParallelotope<N, T> parallelotope_;
        Color color_;
        T pdf_;

        static Vector<N - 1, T> samples(RandomEngine<T>& random_engine)
        {
                std::uniform_real_distribution<T> urd(0, 1);
                Vector<N - 1, T> samples;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        samples[i] = urd(random_engine);
                }
                return samples;
        }

public:
        ParallelotopeLight(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors, const Color& color)
                : parallelotope_(org, vectors), color_(color), pdf_(sampling::uniform_in_parallelotope_pdf(vectors))
        {
        }

        LightSourceSample<N, T, Color> sample(RandomEngine<T>& random_engine, const Vector<N, T>& point) const override
        {
                const Vector<N, T> sample_location =
                        parallelotope_.org()
                        + sampling::uniform_in_parallelotope(parallelotope_.vectors(), samples(random_engine));

                const Vector<N, T> direction = sample_location - point;
                const T distance = direction.norm();
                const Vector<N, T> l = direction / distance;
                const T cos = std::abs(dot(l, parallelotope_.normal()));

                LightSourceSample<N, T, Color> s;

                s.distance = distance;
                s.l = l;
                s.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, distance);
                s.radiance = color_;

                return s;
        }

        T pdf(const Vector<N, T>& point, const Vector<N, T>& l) const override
        {
                const Ray<N, T> ray(point, l);
                auto intersection = parallelotope_.intersect(ray);
                if (!intersection)
                {
                        return 0;
                }
                const T cos = std::abs(dot(ray.dir(), parallelotope_.normal()));
                return sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, *intersection);
        }

        bool is_delta() const override
        {
                return false;
        }
};
}
