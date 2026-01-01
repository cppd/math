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

#include "distant_light.h"

#include "com/functions.h"

#include <src/com/error.h>
#include <src/com/random/pcg.h>
#include <src/geometry/shapes/ball_volume.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void DistantLight<N, T, Color>::init(const numerical::Vector<N, T>& scene_center, const T scene_radius)
{
        if (!(scene_radius > 0))
        {
                error("Scene radius " + to_string(scene_radius) + " must be positive");
        }

        area_ = geometry::shapes::ball_volume<N - 1, T>(scene_radius);

        leave_sample_.ray.set_org(scene_center - scene_radius * leave_sample_.ray.dir());
        leave_sample_.pdf_pos = sampling::uniform_in_sphere_pdf<N - 1>(scene_radius);

        vectors_ =
                com::multiply(numerical::orthogonal_complement_of_unit_vector(leave_sample_.ray.dir()), scene_radius);
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveSample<N, T, Color> DistantLight<N, T, Color>::arrive_sample(
        PCG& /*engine*/,
        const numerical::Vector<N, T>& /*point*/,
        const numerical::Vector<N, T>& /*n*/) const
{
        return arrive_sample_;
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveInfo<T, Color> DistantLight<N, T, Color>::arrive_info(
        const numerical::Vector<N, T>& /*point*/,
        const numerical::Vector<N, T>& /*l*/) const
{
        return LightSourceArriveInfo<T, Color>::non_usable();
}

template <std::size_t N, typename T, typename Color>
LightSourceLeaveSample<N, T, Color> DistantLight<N, T, Color>::leave_sample(PCG& engine) const
{
        LightSourceLeaveSample<N, T, Color> res = leave_sample_;
        res.ray.set_org(leave_sample_.ray.org() + sampling::uniform_in_sphere(engine, vectors_));
        return res;
}

template <std::size_t N, typename T, typename Color>
T DistantLight<N, T, Color>::leave_pdf_pos(const numerical::Vector<N, T>& /*dir*/) const
{
        return leave_sample_.pdf_pos;
}

template <std::size_t N, typename T, typename Color>
T DistantLight<N, T, Color>::leave_pdf_dir(const numerical::Vector<N, T>& /*dir*/) const
{
        return 0;
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> DistantLight<N, T, Color>::leave_radiance(const numerical::Vector<N, T>& /*dir*/) const
{
        return {};
}

template <std::size_t N, typename T, typename Color>
Color DistantLight<N, T, Color>::power() const
{
        ASSERT(area_);

        return *area_ * arrive_sample_.radiance;
}

template <std::size_t N, typename T, typename Color>
bool DistantLight<N, T, Color>::is_delta() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
bool DistantLight<N, T, Color>::is_infinite_area() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
DistantLight<N, T, Color>::DistantLight(const numerical::Vector<N, T>& direction, const Color& radiance)
{
        leave_sample_.ray.set_dir(direction);
        leave_sample_.radiance = radiance;
        leave_sample_.pdf_dir = 1;
        leave_sample_.infinite_distance = true;

        arrive_sample_.l = -leave_sample_.ray.dir();
        arrive_sample_.pdf = 1;
        arrive_sample_.radiance = radiance;
}

#define TEMPLATE(N, T, C) template class DistantLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
