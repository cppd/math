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

#include "infinite_area_light.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/geometry/shapes/ball_volume.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void InfiniteAreaLight<N, T, Color>::init(const Vector<N, T>& /*scene_center*/, const T scene_radius)
{
        if (!(scene_radius > 0))
        {
                error("Scene radius " + to_string(scene_radius) + " must be positive");
        }

        area_ = geometry::ball_volume<N - 1, T>(scene_radius);
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveSample<N, T, Color> InfiniteAreaLight<N, T, Color>::arrive_sample(
        PCG& engine,
        const Vector<N, T>& /*point*/) const
{
        LightSourceArriveSample<N, T, Color> s;
        s.l = sampling::uniform_on_sphere<N, T>(engine);
        s.pdf = sampling::uniform_on_sphere_pdf<N, T>();
        s.radiance = radiance_;
        return s;
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveInfo<T, Color> InfiniteAreaLight<N, T, Color>::arrive_info(
        const Vector<N, T>& /*point*/,
        const Vector<N, T>& /*l*/) const
{
        LightSourceArriveInfo<T, Color> info;
        info.pdf = sampling::uniform_on_sphere_pdf<N, T>();
        info.radiance = radiance_;
        return info;
}

template <std::size_t N, typename T, typename Color>
LightSourceLeaveSample<N, T, Color> InfiniteAreaLight<N, T, Color>::leave_sample(PCG& /*engine*/) const
{
        error("not implemented");
}

template <std::size_t N, typename T, typename Color>
T InfiniteAreaLight<N, T, Color>::leave_pdf_pos(const Vector<N, T>& /*point*/, const Vector<N, T>& /*dir*/) const
{
        error("not implemented");
}

template <std::size_t N, typename T, typename Color>
T InfiniteAreaLight<N, T, Color>::leave_pdf_dir(const Vector<N, T>& /*point*/, const Vector<N, T>& /*dir*/) const
{
        error("not implemented");
}

template <std::size_t N, typename T, typename Color>
Color InfiniteAreaLight<N, T, Color>::power() const
{
        return area_ * radiance_;
}

template <std::size_t N, typename T, typename Color>
bool InfiniteAreaLight<N, T, Color>::is_delta() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
InfiniteAreaLight<N, T, Color>::InfiniteAreaLight(const Color& radiance)
        : radiance_(radiance)
{
}

#define TEMPLATE(N, T, C) template class InfiniteAreaLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
