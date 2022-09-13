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

#include "point_light.h"

#include "com/functions.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/print.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <cmath>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void PointLight<N, T, Color>::init(const Vector<N, T>& /*scene_center*/, const T /*scene_radius*/)
{
}

template <std::size_t N, typename T, typename Color>
Color PointLight<N, T, Color>::radiance(const T squared_distance, const T distance) const
{
        return intensity_ * (1 / com::power_n1<N>(squared_distance, distance));
}

template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> PointLight<N, T, Color>::sample(PCG& /*engine*/, const Vector<N, T>& point) const
{
        const Vector<N, T> direction = location_ - point;
        const T squared_distance = direction.norm_squared();
        const T distance = std::sqrt(squared_distance);

        LightSourceSample<N, T, Color> s;
        s.distance = distance;
        s.l = direction / distance;
        s.pdf = 1;
        s.radiance = radiance(squared_distance, distance);
        return s;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> PointLight<N, T, Color>::info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/) const
{
        LightSourceInfo<T, Color> info;
        info.pdf = 0;
        return info;
}

template <std::size_t N, typename T, typename Color>
LightSourceEmitSample<N, T, Color> PointLight<N, T, Color>::emit_sample(PCG& engine) const
{
        const Ray<N, T> ray(location_, sampling::uniform_on_sphere<N, T>(engine));

        LightSourceEmitSample<N, T, Color> s;
        s.ray = ray;
        s.pdf_pos = 1;
        s.pdf_dir = sampling::uniform_on_sphere_pdf<N, T>();
        s.radiance = intensity_;
        return s;
}

template <std::size_t N, typename T, typename Color>
T PointLight<N, T, Color>::emit_pdf_pos(const Vector<N, T>& /*point*/, const Vector<N, T>& /*dir*/) const
{
        return 0;
}

template <std::size_t N, typename T, typename Color>
T PointLight<N, T, Color>::emit_pdf_dir(const Vector<N, T>& /*point*/, const Vector<N, T>& /*dir*/) const
{
        return sampling::uniform_on_sphere_pdf<N, T>();
}

template <std::size_t N, typename T, typename Color>
Color PointLight<N, T, Color>::power() const
{
        return geometry::SPHERE_AREA<N, T> * intensity_;
}

template <std::size_t N, typename T, typename Color>
bool PointLight<N, T, Color>::is_delta() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
PointLight<N, T, Color>::PointLight(
        const Vector<N, T>& location,
        const Color& radiance,
        const std::type_identity_t<T> radiance_distance)
        : location_(location),
          intensity_(radiance * ns::power<N - 1>(radiance_distance))
{
        if (!(radiance_distance > 0))
        {
                error("Error radiance distance " + to_string(radiance_distance));
        }
}

#define TEMPLATE(N, T, C) template class PointLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
