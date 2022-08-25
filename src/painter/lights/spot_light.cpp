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

#include "spot_light.h"

#include "com/functions.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <cmath>

namespace ns::painter::lights
{
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
LightSourceSample<N, T, Color> SpotLight<N, T, Color>::sample(PCG& /*engine*/, const Vector<N, T>& point) const
{
        const Vector<N, T> direction = location_ - point;
        const T squared_distance = direction.norm_squared();
        const T distance = std::sqrt(squared_distance);
        const Vector<N, T> l = direction / distance;
        const T cos = -dot(l, direction_);

        LightSourceSample<N, T, Color> s;
        s.distance = distance;
        s.l = l;
        s.pdf = 1;
        s.radiance = radiance(cos, squared_distance, distance);
        return s;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> SpotLight<N, T, Color>::info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const
{
        LightSourceInfo<T, Color> info;
        info.pdf = 0;
        return info;
}

template <std::size_t N, typename T, typename Color>
LightSourceEmitSample<N, T, Color> SpotLight<N, T, Color>::emit_sample(PCG& engine) const
{
        const Ray<N, T> ray(location_, sampling::uniform_on_sphere<N, T>(engine));
        const T cos = dot(direction_, ray.dir());

        LightSourceEmitSample<N, T, Color> s;
        s.ray = ray;
        s.pdf_pos = 1;
        s.pdf_dir = sampling::uniform_on_sphere_pdf<N, T>();
        s.radiance = spotlight_.color(intensity_, cos);
        return s;
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
}

#define TEMPLATE(N, T, C) template class SpotLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
