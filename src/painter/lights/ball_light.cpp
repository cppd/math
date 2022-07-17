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

#include "ball_light.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/complement.h>
#include <src/sampling/pdf.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <cmath>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> BallLight<N, T, Color>::sample(PCG& engine, const Vector<N, T>& point) const
{
        if (dot(ball_.normal(), point - ball_.center()) <= 0)
        {
                LightSourceSample<N, T, Color> sample;
                sample.pdf = 0;
                return sample;
        }

        const Vector<N, T> sample_location = ball_.center() + sampling::uniform_in_sphere(vectors_, engine);

        const Vector<N, T> direction = sample_location - point;
        const T distance = direction.norm();
        const Vector<N, T> l = direction / distance;

        const T cos = std::abs(dot(l, ball_.normal()));

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
LightSourceInfo<T, Color> BallLight<N, T, Color>::info(const Vector<N, T>& point, const Vector<N, T>& l) const
{
        if (dot(ball_.normal(), point - ball_.center()) <= 0)
        {
                LightSourceInfo<T, Color> info;
                info.pdf = 0;
                return info;
        }

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
bool BallLight<N, T, Color>::is_delta() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
BallLight<N, T, Color>::BallLight(
        const Vector<N, T>& center,
        const Vector<N, T>& direction,
        const std::type_identity_t<T> radius,
        const Color& color)
        : ball_(center, direction, radius),
          color_(color),
          pdf_(sampling::uniform_in_sphere_pdf<std::tuple_size_v<decltype(vectors_)>>(radius)),
          vectors_(numerical::orthogonal_complement_of_unit_vector(ball_.normal()))
{
        if (!(radius > 0))
        {
                error("Ball light radius " + to_string(radius) + " must be positive");
        }

        for (Vector<N, T>& v : vectors_)
        {
                v *= radius;
        }
}

template <std::size_t N, typename T, typename Color>
BallLight<N, T, Color>::BallLight(
        const Vector<N, T>& center,
        const Vector<N, T>& direction,
        const std::type_identity_t<T> radius,
        const Color& color,
        const std::type_identity_t<T> spotlight_falloff_start,
        const std::type_identity_t<T> spotlight_width)
        : BallLight(center, direction, radius, color)
{
        if (!(spotlight_width <= 90))
        {
                error("Ball spotlight width " + to_string(spotlight_width) + " must be less than or equal to 90");
        }

        spotlight_.emplace(spotlight_falloff_start, spotlight_width);
}

template <std::size_t N, typename T, typename Color>
void BallLight<N, T, Color>::set_color_for_distance(const T distance)
{
        if (!(distance > 0))
        {
                error("Ball light distance " + to_string(distance) + " must be positive");
        }

        color_ *= sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, T{1} /*cosine*/, distance);
}

#define TEMPLATE(N, T, C) template class BallLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
