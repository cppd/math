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

#include "ball_light.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/geometry/shapes/ball_volume.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/sampling/pdf.h>
#include <src/sampling/sphere_cosine.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void BallLight<N, T, Color>::init(const Vector<N, T>& /*scene_center*/, const T /*scene_radius*/)
{
}

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
LightSourceArriveSample<N, T, Color> BallLight<N, T, Color>::arrive_sample(
        PCG& engine,
        const Vector<N, T>& point,
        const Vector<N, T>& /*n*/) const
{
        if (!visible(point))
        {
                LightSourceArriveSample<N, T, Color> res;
                res.pdf = 0;
                return res;
        }

        const Vector<N, T> direction = sample_location(engine) - point;
        const T distance = direction.norm();
        const Vector<N, T> l = direction / distance;

        const T cos = -dot(l, ball_.normal());

        LightSourceArriveSample<N, T, Color> res;
        res.l = l;
        res.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, distance);
        res.radiance = radiance(cos);
        res.distance = distance;
        return res;
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveInfo<T, Color> BallLight<N, T, Color>::arrive_info(const Vector<N, T>& point, const Vector<N, T>& l)
        const
{
        if (!visible(point))
        {
                LightSourceArriveInfo<T, Color> res;
                res.pdf = 0;
                return res;
        }

        const Ray<N, T> ray(point, l);
        const auto intersection = ball_.intersect(ray);
        if (!intersection)
        {
                LightSourceArriveInfo<T, Color> res;
                res.pdf = 0;
                return res;
        }

        const T cos = -dot(ray.dir(), ball_.normal());

        LightSourceArriveInfo<T, Color> res;
        res.pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, *intersection);
        res.radiance = radiance(cos);
        res.distance = *intersection;
        return res;
}

template <std::size_t N, typename T, typename Color>
LightSourceLeaveSample<N, T, Color> BallLight<N, T, Color>::leave_sample(PCG& engine) const
{
        const Ray<N, T> ray(sample_location(engine), sampling::cosine_on_hemisphere(engine, ball_.normal()));
        const T cos = dot(ball_.normal(), ray.dir());

        LightSourceLeaveSample<N, T, Color> res;
        res.ray = ray;
        res.n = ball_.normal();
        res.pdf_pos = pdf_;
        res.pdf_dir = sampling::cosine_on_hemisphere_pdf<N, T>(cos);
        res.radiance = radiance(cos);
        res.infinite_distance = false;
        return res;
}

template <std::size_t N, typename T, typename Color>
T BallLight<N, T, Color>::leave_pdf_pos(const Vector<N, T>& /*dir*/) const
{
        return pdf_;
}

template <std::size_t N, typename T, typename Color>
T BallLight<N, T, Color>::leave_pdf_dir(const Vector<N, T>& dir) const
{
        ASSERT(dir.is_unit());
        const T cos = dot(ball_.normal(), dir);
        return sampling::cosine_on_hemisphere_pdf<N, T>(cos);
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> BallLight<N, T, Color>::leave_radiance(const Vector<N, T>& dir) const
{
        ASSERT(dir.is_unit());
        const T cos = dot(dir, ball_.normal());
        if (cos > 0)
        {
                return radiance(cos);
        }
        return {};
}

template <std::size_t N, typename T, typename Color>
Color BallLight<N, T, Color>::power() const
{
        const T cosine_integral =
                spotlight_ ? spotlight_->cosine_integral()
                           : geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, T>;
        return (area_ * cosine_integral) * radiance_;
}

template <std::size_t N, typename T, typename Color>
bool BallLight<N, T, Color>::is_delta() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
bool BallLight<N, T, Color>::is_infinite_area() const
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
          pdf_(sampling::uniform_in_sphere_pdf<N - 1>(radius)),
          area_(geometry::shapes::ball_volume<N - 1, T>(radius)),
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
