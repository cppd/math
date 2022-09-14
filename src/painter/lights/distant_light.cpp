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

#include "distant_light.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/geometry/shapes/ball_volume.h>
#include <src/numerical/complement.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void DistantLight<N, T, Color>::init(const Vector<N, T>& scene_center, const T scene_radius)
{
        if (!(scene_radius > 0))
        {
                error("Scene radius " + to_string(scene_radius) + " must be positive");
        }

        area_ = geometry::ball_volume<N - 1, T>(scene_radius);

        emit_sample_.ray.set_org(scene_center - scene_radius * emit_sample_.ray.dir());
        emit_sample_.pdf_pos = sampling::uniform_in_sphere_pdf<N - 1>(scene_radius);

        vectors_ = numerical::orthogonal_complement_of_unit_vector(emit_sample_.ray.dir());
        for (Vector<N, T>& v : vectors_)
        {
                v *= scene_radius;
        }
}

template <std::size_t N, typename T, typename Color>
LightSourceSample<N, T, Color> DistantLight<N, T, Color>::sample(PCG& /*engine*/, const Vector<N, T>& /*point*/) const
{
        return sample_;
}

template <std::size_t N, typename T, typename Color>
LightSourceInfo<T, Color> DistantLight<N, T, Color>::info(const Vector<N, T>& /*point*/, const Vector<N, T>& /*l*/)
        const
{
        LightSourceInfo<T, Color> info;
        info.pdf = 0;
        return info;
}

template <std::size_t N, typename T, typename Color>
LightSourceEmitSample<N, T, Color> DistantLight<N, T, Color>::emit_sample(PCG& engine) const
{
        LightSourceEmitSample<N, T, Color> s = emit_sample_;
        s.ray.set_org(emit_sample_.ray.org() + sampling::uniform_in_sphere(engine, vectors_));
        return s;
}

template <std::size_t N, typename T, typename Color>
T DistantLight<N, T, Color>::emit_pdf_pos(const Vector<N, T>& /*point*/, const Vector<N, T>& /*dir*/) const
{
        return emit_sample_.pdf_pos;
}

template <std::size_t N, typename T, typename Color>
T DistantLight<N, T, Color>::emit_pdf_dir(const Vector<N, T>& /*point*/, const Vector<N, T>& /*dir*/) const
{
        return 0;
}

template <std::size_t N, typename T, typename Color>
Color DistantLight<N, T, Color>::power() const
{
        return area_ * sample_.radiance;
}

template <std::size_t N, typename T, typename Color>
bool DistantLight<N, T, Color>::is_delta() const
{
        return true;
}

template <std::size_t N, typename T, typename Color>
DistantLight<N, T, Color>::DistantLight(const Vector<N, T>& direction, const Color& radiance)
{
        emit_sample_.ray.set_dir(direction);
        emit_sample_.radiance = radiance;
        emit_sample_.pdf_dir = 1;

        sample_.l = -emit_sample_.ray.dir();
        sample_.pdf = 1;
        sample_.radiance = radiance;
}

#define TEMPLATE(N, T, C) template class DistantLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
