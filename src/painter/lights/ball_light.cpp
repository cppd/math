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
#include <src/sampling/sphere_cosine.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
bool BallLight<N, T, Color>::visible(const Vector<N, T>& point) const
{
        return dot(ball_.normal(), point - ball_.center()) > 0;
}

template <std::size_t N, typename T, typename Color>
Vector<N, T> BallLight<N, T, Color>::sample_location(PCG& engine) const
{
        return ball_.center() + sampling::uniform_in_sphere(engine, vectors_);
}

template <std::size_t N, typename T, typename Color>
Color BallLight<N, T, Color>::radiance(const T cos) const
{
        if (!spotlight_)
        {
                return radiance_;
        }
        return spotlight_->color(radiance_, cos);
}

template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> BallLight<N, T, Color>::sample(PCG& engine, const Vector<N, T>& point) const
{
        if (!visible(point))
        {
                LightSourceSample<N, T, Color> sample;
                sample.pdf = 0;
                return sample;
        }

        const Vector<N, T> direction = sample_location(engine) - point;
        const T distance = direction.norm();
        const Vector<N, T> l = direction / distance;

        const T cos = -dot(l, ball_.normal());

        LightSourceSample<N, T, Color> s;
        s.l = l;
        s.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, distance);
        s.radiance = radiance(cos);
        s.distance = distance;
        return s;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> BallLight<N, T, Color>::info(const Vector<N, T>& point, const Vector<N, T>& l) const
{
        if (!visible(point))
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

        const T cos = -dot(ray.dir(), ball_.normal());

        LightSourceInfo<T, Color> info;
        info.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, *intersection);
        info.radiance = radiance(cos);
        info.distance = *intersection;
        return info;
}

template <std::size_t N, typename T, typename Color>
LightSourceSampleEmit<N, T, Color> BallLight<N, T, Color>::sample_emit(PCG& engine) const
{
        const Ray<N, T> ray(sample_location(engine), sampling::cosine_on_hemisphere(engine, ball_.normal()));
        const T cos = dot(ball_.normal(), ray.dir());

        LightSourceSampleEmit<N, T, Color> s;
        s.ray = ray;
        s.n = ball_.normal();
        s.pdf_pos = pdf_;
        s.pdf_dir = sampling::cosine_on_hemisphere_pdf<N, T>(cos);
        s.radiance = radiance(cos);
        return s;
}

template <std::size_t N, typename T, typename Color>
Color BallLight<N, T, Color>::power() const
{
        error("not implemented");
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
        const Color& radiance)
        : ball_(center, direction, radius),
          radiance_(radiance),
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
        const Color& radiance,
        const std::type_identity_t<T> spotlight_falloff_start,
        const std::type_identity_t<T> spotlight_width)
        : BallLight(center, direction, radius, radiance)
{
        if (!(spotlight_width <= 90))
        {
                error("Ball spotlight width " + to_string(spotlight_width) + " must be less than or equal to 90");
        }

        spotlight_.emplace(spotlight_falloff_start, spotlight_width);
}

template <std::size_t N, typename T, typename Color>
void BallLight<N, T, Color>::set_radiance_for_distance(const T distance)
{
        if (!(distance > 0))
        {
                error("Ball light distance " + to_string(distance) + " must be positive");
        }

        radiance_ *= sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, T{1} /*cosine*/, distance);
}

#define TEMPLATE(N, T, C) template class BallLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
