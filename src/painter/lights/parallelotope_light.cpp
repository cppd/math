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

#include "parallelotope_light.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/sampling/parallelotope_uniform.h>
#include <src/sampling/pdf.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <cmath>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> ParallelotopeLight<N, T, Color>::sample(PCG& engine, const Vector<N, T>& point) const
{
        if (dot(parallelotope_.normal(), point - parallelotope_.org()) <= 0)
        {
                LightSourceSample<N, T, Color> sample;
                sample.pdf = 0;
                return sample;
        }

        const Vector<N, T> sample_location =
                parallelotope_.org() + sampling::uniform_in_parallelotope(parallelotope_.vectors(), engine);

        const Vector<N, T> direction = sample_location - point;
        const T distance = direction.norm();
        const Vector<N, T> l = direction / distance;

        const T cos = std::abs(dot(l, parallelotope_.normal()));

        LightSourceSample<N, T, Color> s;
        s.l = l;
        s.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, distance);
        if (!spotlight_)
        {
                s.radiance = color_;
        }
        else
        {
                s.radiance = spotlight_->color(color_, cos);
        }
        s.distance = distance;
        return s;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> ParallelotopeLight<N, T, Color>::info(const Vector<N, T>& point, const Vector<N, T>& l) const
{
        if (dot(parallelotope_.normal(), point - parallelotope_.org()) <= 0)
        {
                LightSourceInfo<T, Color> info;
                info.pdf = 0;
                return info;
        }

        const Ray<N, T> ray(point, l);
        const auto intersection = parallelotope_.intersect(ray);
        if (!intersection)
        {
                LightSourceInfo<T, Color> info;
                info.pdf = 0;
                return info;
        }

        const T cos = std::abs(dot(ray.dir(), parallelotope_.normal()));

        LightSourceInfo<T, Color> info;
        info.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, *intersection);
        if (!spotlight_)
        {
                info.radiance = color_;
        }
        else
        {
                info.radiance = spotlight_->color(color_, cos);
        }
        info.distance = *intersection;
        return info;
}

template <std::size_t N, typename T, typename Color>
LightSourceSampleEmit<N, T, Color> ParallelotopeLight<N, T, Color>::sample_emit(PCG& /*engine*/) const
{
        error("not implemented");
}

template <std::size_t N, typename T, typename Color>
bool ParallelotopeLight<N, T, Color>::is_delta() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
ParallelotopeLight<N, T, Color>::ParallelotopeLight(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N - 1>& vectors,
        const Vector<N, T>& direction,
        const Color& color)
        : parallelotope_(org, vectors),
          color_(color),
          pdf_(sampling::uniform_in_parallelotope_pdf(vectors))
{
        if (!std::all_of(
                    vectors.cbegin(), vectors.cend(),
                    [](const Vector<N, T>& v)
                    {
                            return v.norm_squared() > 0;
                    }))
        {
                error("Parallelotope vectors " + to_string(vectors) + " must be non-zero");
        }

        parallelotope_.set_normal_direction(direction);
}

template <std::size_t N, typename T, typename Color>
ParallelotopeLight<N, T, Color>::ParallelotopeLight(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N - 1>& vectors,
        const Vector<N, T>& direction,
        const Color& color,
        const std::type_identity_t<T> spotlight_falloff_start,
        const std::type_identity_t<T> spotlight_width)
        : ParallelotopeLight(org, vectors, direction, color)
{
        if (!(spotlight_width <= 90))
        {
                error("Parallolotope spotlight width " + to_string(spotlight_width)
                      + " must be less than or equal to 90");
        }

        spotlight_.emplace(spotlight_falloff_start, spotlight_width);
}

#define TEMPLATE(N, T, C) template class ParallelotopeLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
