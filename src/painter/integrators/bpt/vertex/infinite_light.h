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

#pragma once

#include "area_pdf.h"

#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/integrators/bpt/light_distribution.h>
#include <src/painter/objects.h>

#include <cstddef>

namespace ns::painter::integrators::bpt::vertex
{
namespace infinite_light_implementation
{
template <std::size_t N, typename T, typename Color>
[[nodiscard]] T angle_pdf_origin(
        const Scene<N, T, Color>& scene,
        const LightDistribution<N, T, Color>& light_distribution,
        const Ray<N, T>& ray_to_light)
{
        T sum = 0;
        T weight_sum = 0;

        for (const LightSource<N, T, Color>* const light : scene.light_sources())
        {
                if (!light->is_infinite_area())
                {
                        continue;
                }

                const T pdf_dir = light->leave_pdf_dir(-ray_to_light.dir());
                const T distribution_pdf = light_distribution.pdf(light);
                sum += pdf_dir * distribution_pdf;
                weight_sum += distribution_pdf;
        }

        if (weight_sum > 0)
        {
                return sum / weight_sum;
        }
        return 0;
}

template <std::size_t N, typename T, typename Color, typename Normal>
[[nodiscard]] T area_pdf(
        const Scene<N, T, Color>& scene,
        const LightDistribution<N, T, Color>& light_distribution,
        const Vector<N, T>& light_dir,
        const Normal& next_normal)
{
        T sum = 0;
        T weight_sum = 0;

        for (const LightSource<N, T, Color>* const light : scene.light_sources())
        {
                if (!light->is_infinite_area())
                {
                        continue;
                }

                const T pdf = light->leave_pdf_pos(light_dir);
                const T distribution_pdf = light_distribution.pdf(light);
                sum += pdf * distribution_pdf;
                weight_sum += distribution_pdf;
        }

        if (weight_sum > 0)
        {
                return pos_pdf_to_area_pdf(sum / weight_sum, light_dir, next_normal);
        }
        return 0;
}
}

template <std::size_t N, typename T, typename Color>
class InfiniteLight final
{
        const Scene<N, T, Color>* scene_;
        const LightDistribution<N, T, Color>* light_distribution_;
        Vector<N, T> dir_;
        Color beta_;
        T angle_pdf_forward_;
        T angle_pdf_origin_;

public:
        InfiniteLight(
                const Scene<N, T, Color>* const scene,
                const LightDistribution<N, T, Color>* const light_distribution,
                const Ray<N, T>& ray_to_light,
                const Color& beta,
                const T angle_pdf_forward)
                : scene_(scene),
                  light_distribution_(light_distribution),
                  dir_(-ray_to_light.dir()),
                  beta_(beta),
                  angle_pdf_forward_(angle_pdf_forward),
                  angle_pdf_origin_(
                          infinite_light_implementation::angle_pdf_origin(*scene, *light_distribution, ray_to_light))
        {
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }

        [[nodiscard]] const Vector<N, T>& dir() const
        {
                return dir_;
        }

        template <typename Normal>
        [[nodiscard]] T area_pdf(const Normal& next_normal) const
        {
                return infinite_light_implementation::area_pdf(*scene_, *light_distribution_, dir_, next_normal);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return true;
        }

        [[nodiscard]] T pdf_origin() const
        {
                return angle_pdf_origin_;
        }

        [[nodiscard]] T pdf_forward() const
        {
                return angle_pdf_forward_;
        }
};
}
