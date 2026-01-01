/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "com/functions.h"

#include <src/com/error.h>
#include <src/com/random/pcg.h>
#include <src/geometry/shapes/ball_volume.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void InfiniteAreaLight<N, T, Color>::init(const numerical::Vector<N, T>& scene_center, const T scene_radius)
{
        if (!(scene_radius > 0))
        {
                error("Scene radius " + to_string(scene_radius) + " must be positive");
        }

        area_ = geometry::shapes::ball_volume<N - 1, T>(scene_radius);

        scene_center_ = scene_center;
        scene_radius_ = scene_radius;
        leave_pdf_pos_ = sampling::uniform_in_sphere_pdf<N - 1>(scene_radius);
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveSample<N, T, Color> InfiniteAreaLight<N, T, Color>::arrive_sample(
        PCG& engine,
        const numerical::Vector<N, T>& /*point*/,
        const numerical::Vector<N, T>& n) const
{
        const numerical::Vector<N, T> l = sampling::uniform_on_sphere<N, T>(engine);

        return {
                .l = (dot(n, l) >= 0) ? l : -l,
                .pdf = sampling::uniform_on_hemisphere_pdf<N, T>(),
                .radiance = radiance_,
                .distance = std::nullopt,
        };
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveInfo<T, Color> InfiniteAreaLight<N, T, Color>::arrive_info(
        const numerical::Vector<N, T>& /*point*/,
        const numerical::Vector<N, T>& /*l*/) const
{
        return {
                .pdf = sampling::uniform_on_hemisphere_pdf<N, T>(),
                .radiance = radiance_,
                .distance = std::nullopt,
        };
}

template <std::size_t N, typename T, typename Color>
LightSourceLeaveSample<N, T, Color> InfiniteAreaLight<N, T, Color>::leave_sample(PCG& engine) const
{
        const numerical::Vector<N, T> dir = sampling::uniform_on_sphere<N, T>(engine);

        const std::array<numerical::Vector<N, T>, N - 1> vectors =
                com::multiply(numerical::orthogonal_complement_of_unit_vector(dir), scene_radius_);

        const numerical::Vector<N, T> org =
                scene_center_ - scene_radius_ * dir + sampling::uniform_in_sphere(engine, vectors);

        return {
                .ray{org, dir},
                .n = std::nullopt,
                .pdf_pos = leave_pdf_pos_,
                .pdf_dir = leave_pdf_dir_,
                .radiance = radiance_,
                .infinite_distance = true,
        };
}

template <std::size_t N, typename T, typename Color>
T InfiniteAreaLight<N, T, Color>::leave_pdf_pos(const numerical::Vector<N, T>& /*dir*/) const
{
        return leave_pdf_pos_;
}

template <std::size_t N, typename T, typename Color>
T InfiniteAreaLight<N, T, Color>::leave_pdf_dir(const numerical::Vector<N, T>& /*dir*/) const
{
        return leave_pdf_dir_;
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> InfiniteAreaLight<N, T, Color>::leave_radiance(const numerical::Vector<N, T>& /*dir*/) const
{
        return radiance_;
}

template <std::size_t N, typename T, typename Color>
Color InfiniteAreaLight<N, T, Color>::power() const
{
        ASSERT(area_);

        return *area_ * radiance_;
}

template <std::size_t N, typename T, typename Color>
bool InfiniteAreaLight<N, T, Color>::is_delta() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
bool InfiniteAreaLight<N, T, Color>::is_infinite_area() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
InfiniteAreaLight<N, T, Color>::InfiniteAreaLight(const Color& radiance)
        : radiance_(radiance),
          leave_pdf_dir_(sampling::uniform_on_sphere_pdf<N, T>())
{
}

#define TEMPLATE(N, T, C) template class InfiniteAreaLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
