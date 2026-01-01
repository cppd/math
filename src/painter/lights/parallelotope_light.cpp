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

#include "parallelotope_light.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/geometry/shapes/parallelotope_volume.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/geometry/spatial/hyperplane_parallelotope.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/sampling/parallelotope_uniform.h>
#include <src/sampling/pdf.h>
#include <src/sampling/sphere_cosine.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::painter::lights
{
template <std::size_t N, typename T, typename Color>
void ParallelotopeLight<N, T, Color>::init(const numerical::Vector<N, T>& /*scene_center*/, const T /*scene_radius*/)
{
}

template <std::size_t N, typename T, typename Color>
bool ParallelotopeLight<N, T, Color>::visible(const numerical::Vector<N, T>& point) const
{
        return dot(parallelotope_.normal(), point - parallelotope_.org()) > 0;
}

template <std::size_t N, typename T, typename Color>
numerical::Vector<N, T> ParallelotopeLight<N, T, Color>::sample_location(PCG& engine) const
{
        return parallelotope_.org() + sampling::uniform_in_parallelotope(engine, parallelotope_.vectors());
}

template <std::size_t N, typename T, typename Color>
Color ParallelotopeLight<N, T, Color>::radiance(const T cos) const
{
        if (!spotlight_)
        {
                return radiance_;
        }
        return spotlight_->color(radiance_, cos);
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveSample<N, T, Color> ParallelotopeLight<N, T, Color>::arrive_sample(
        PCG& engine,
        const numerical::Vector<N, T>& point,
        const numerical::Vector<N, T>& /*n*/) const
{
        if (!visible(point))
        {
                return LightSourceArriveSample<N, T, Color>::non_usable();
        }

        const numerical::Vector<N, T> direction = sample_location(engine) - point;
        const T distance = direction.norm();
        const numerical::Vector<N, T> l = direction / distance;

        const T cos = -dot(l, parallelotope_.normal());

        return {
                .l = l,
                .pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, distance),
                .radiance = radiance(cos),
                .distance = distance,
        };
}

template <std::size_t N, typename T, typename Color>
LightSourceArriveInfo<T, Color> ParallelotopeLight<N, T, Color>::arrive_info(
        const numerical::Vector<N, T>& point,
        const numerical::Vector<N, T>& l) const
{
        if (!visible(point))
        {
                return LightSourceArriveInfo<T, Color>::non_usable();
        }

        const numerical::Ray<N, T> ray(point, l);
        const auto intersection = parallelotope_.intersect(ray);
        if (!intersection)
        {
                return LightSourceArriveInfo<T, Color>::non_usable();
        }

        const T cos = -dot(ray.dir(), parallelotope_.normal());

        return {
                .pdf = sampling::area_pdf_to_solid_angle_pdf<N>(pdf_, cos, *intersection),
                .radiance = radiance(cos),
                .distance = intersection,
        };
}

template <std::size_t N, typename T, typename Color>
LightSourceLeaveSample<N, T, Color> ParallelotopeLight<N, T, Color>::leave_sample(PCG& engine) const
{
        const numerical::Ray<N, T> ray(
                sample_location(engine), sampling::cosine_on_hemisphere(engine, parallelotope_.normal()));
        const T cos = dot(parallelotope_.normal(), ray.dir());

        return {
                .ray = ray,
                .n = parallelotope_.normal(),
                .pdf_pos = pdf_,
                .pdf_dir = sampling::cosine_on_hemisphere_pdf<N, T>(cos),
                .radiance = radiance(cos),
                .infinite_distance = false,
        };
}

template <std::size_t N, typename T, typename Color>
T ParallelotopeLight<N, T, Color>::leave_pdf_pos(const numerical::Vector<N, T>& /*dir*/) const
{
        return pdf_;
}

template <std::size_t N, typename T, typename Color>
T ParallelotopeLight<N, T, Color>::leave_pdf_dir(const numerical::Vector<N, T>& dir) const
{
        ASSERT(dir.is_unit());
        const T cos = dot(parallelotope_.normal(), dir);
        return sampling::cosine_on_hemisphere_pdf<N, T>(cos);
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> ParallelotopeLight<N, T, Color>::leave_radiance(const numerical::Vector<N, T>& dir) const
{
        ASSERT(dir.is_unit());
        const T cos = dot(dir, parallelotope_.normal());
        if (cos > 0)
        {
                return radiance(cos);
        }
        return {};
}

template <std::size_t N, typename T, typename Color>
Color ParallelotopeLight<N, T, Color>::power() const
{
        const T area = geometry::shapes::parallelotope_volume(parallelotope_.vectors());
        const T cosine_integral =
                spotlight_ ? spotlight_->cosine_integral()
                           : geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, T>;
        return (area * cosine_integral) * radiance_;
}

template <std::size_t N, typename T, typename Color>
bool ParallelotopeLight<N, T, Color>::is_delta() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
bool ParallelotopeLight<N, T, Color>::is_infinite_area() const
{
        return false;
}

template <std::size_t N, typename T, typename Color>
ParallelotopeLight<N, T, Color>::ParallelotopeLight(
        const geometry::spatial::HyperplaneParallelotope<N, T>& parallelotope,
        const numerical::Vector<N, T>& direction,
        const Color& radiance)
        : parallelotope_(parallelotope),
          radiance_(radiance),
          pdf_(sampling::uniform_in_parallelotope_pdf(parallelotope_.vectors()))
{
        parallelotope_.set_normal_direction(direction);
}

template <std::size_t N, typename T, typename Color>
ParallelotopeLight<N, T, Color>::ParallelotopeLight(
        const geometry::spatial::HyperplaneParallelotope<N, T>& parallelotope,
        const numerical::Vector<N, T>& direction,
        const Color& radiance,
        const std::type_identity_t<T> spotlight_falloff_start,
        const std::type_identity_t<T> spotlight_width)
        : ParallelotopeLight(parallelotope, direction, radiance)
{
        if (!(spotlight_width <= 90))
        {
                error("Parallolotope spotlight width " + to_string(spotlight_width)
                      + " must be less than or equal to 90");
        }

        spotlight_.emplace(spotlight_falloff_start, spotlight_width);
}

#define TEMPLATE(N, T, C) template class ParallelotopeLight<(N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
