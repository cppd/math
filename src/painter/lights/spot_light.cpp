/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "spot_light.h"

#include "com/functions.h"

#include "../objects.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void SpotLight<N, T, Color>::init(const Vector<N, T>& /*scene_center*/, const T /*scene_radius*/)
{
}

template <std::size_t N, typename T, typename Color>
Color SpotLight<N, T, Color>::radiance(const T cos, const T squared_distance, const T distance) const
{
        const T spotlight_coef = spotlight_.coef(cos);
        if (spotlight_coef <= 0)
        {
                return Color(0);
        }
        return intensity_ * (spotlight_coef / com::power_n1<N>(squared_distance, distance));
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveSample<N, T, Color> SpotLight<N, T, Color>::arrive_sample(
        PCG& /*engine*/,
        const Vector<N, T>& point,
        const Vector<N, T>& /*n*/) const
{
        const Vector<N, T> direction = location_ - point;
        const T squared_distance = direction.norm_squared();
        const T distance = std::sqrt(squared_distance);
        const Vector<N, T> l = direction / distance;
        const T cos = -dot(l, direction_);

        LightSourceArriveSample<N, T, Color> res;
        res.distance = distance;
        res.l = l;
        res.pdf = 1;
        res.radiance = radiance(cos, squared_distance, distance);
        return res;
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveInfo<T, Color> SpotLight<N, T, Color>::arrive_info(
        const Vector<N, T>& /*point*/,
        const Vector<N, T>& /*l*/) const
{
        LightSourceArriveInfo<T, Color> res;
        res.pdf = 0;
        return res;
}

template <std::size_t N, typename T, typename Color>
LightSourceLeaveSample<N, T, Color> SpotLight<N, T, Color>::leave_sample(PCG& engine) const
{
        const Ray<N, T> ray = [&]
        {
                const Ray<N, T> r(location_, sampling::uniform_on_sphere<N, T>(engine));
                return (dot(r.dir(), direction_) >= 0) ? r : r.reversed();
        }();
        const T cos = dot(direction_, ray.dir());

        LightSourceLeaveSample<N, T, Color> res;
        res.ray = ray;
        res.pdf_pos = 1;
        res.pdf_dir = sampling::uniform_on_hemisphere_pdf<N, T>();
        res.radiance = spotlight_.color(intensity_, cos);
        res.infinite_distance = false;
        return res;
}

template <std::size_t N, typename T, typename Color>
T SpotLight<N, T, Color>::leave_pdf_pos(const Vector<N, T>& /*dir*/) const
{
        return 0;
}

template <std::size_t N, typename T, typename Color>
T SpotLight<N, T, Color>::leave_pdf_dir(const Vector<N, T>& dir) const
{
        ASSERT(dir.is_unit());
        return dot(dir, direction_) >= 0 ? sampling::uniform_on_hemisphere_pdf<N, T>() : 0;
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> SpotLight<N, T, Color>::leave_radiance(const Vector<N, T>& /*dir*/) const
{
        return {};
}

template <std::size_t N, typename T, typename Color>
Color SpotLight<N, T, Color>::power() const
{
        return spotlight_.area() * intensity_;
}

template <std::size_t N, typename T, typename Color>
bool SpotLight<N, T, Color>::is_delta() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
bool SpotLight<N, T, Color>::is_infinite_area() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
SpotLight<N, T, Color>::SpotLight(
        const Vector<N, T>& location,
        const Vector<N, T>& direction,
        const Color& radiance,
        const std::type_identity_t<T> radiance_distance,
        const std::type_identity_t<T> falloff_start,
        const std::type_identity_t<T> width)
        : location_(location),
          direction_(direction.normalized()),
          intensity_(radiance * ns::power<N - 1>(radiance_distance)),
          spotlight_(falloff_start, width)
{
        if (!(radiance_distance > 0))
        {
                error("Error radiance distance " + to_string(radiance_distance));
        }

        if (!(falloff_start >= 0 && width > 0 && falloff_start <= width && width <= 90))
        {
                error("Error falloff start " + to_string(falloff_start) + " and width " + to_string(width));
        }
}

#define TEMPLATE(N, T, C) template class SpotLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
