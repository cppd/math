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

#pragma once

#include "light_distribution.h"
#include "probability_density.h"

#include "../../objects.h"
#include "../com/normals.h"

#include <src/com/error.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <optional>
#include <variant>

namespace ns::painter::integrators::bpt
{
template <std::size_t N, typename T, typename Color>
class Surface final
{
        SurfaceIntersection<N, T, Color> surface_;
        Normals<N, T> normals_;
        Color beta_;
        Vector<N, T> dir_to_prev_;
        T pdf_forward_ = 0;
        T pdf_reversed_ = 0;

public:
        Surface(const SurfaceIntersection<N, T, Color>& surface,
                const Normals<N, T>& normals,
                const Color& beta,
                const Vector<N, T>& dir_to_prev)
                : surface_(surface),
                  normals_(normals),
                  beta_(beta),
                  dir_to_prev_(dir_to_prev)
        {
        }

        [[nodiscard]] const Vector<N, T>& dir_to_prev() const
        {
                return dir_to_prev_;
        }

        [[nodiscard]] const Vector<N, T>& pos() const
        {
                return surface_.point();
        }

        [[nodiscard]] const Vector<N, T>& normal() const
        {
                return normals_.shading;
        }

        [[nodiscard]] const Normals<N, T>& normals() const
        {
                return normals_;
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }

        template <typename Normal>
        [[nodiscard]] T area_pdf(const T angle_pdf, const Vector<N, T>& next_pos, const Normal& next_normal) const
        {
                return solid_angle_pdf_to_area_pdf(surface_.point(), angle_pdf, next_pos, next_normal);
        }

        template <typename Prev>
        void set_forward_pdf(const Prev& prev, const T angle_pdf)
        {
                pdf_forward_ = prev.area_pdf(angle_pdf, surface_.point(), normals_.shading);
        }

        void set_reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf)
        {
                pdf_reversed_ = next.area_pdf(angle_pdf, surface_.point(), normals_.shading);
        }

        void set_reversed_area_pdf(const T pdf)
        {
                pdf_reversed_ = pdf;
        }

        template <typename Normal>
        [[nodiscard]] T area_pdf(
                const Vector<N, T>& dir_to_previous,
                const Vector<N, T>& next_pos,
                const Normal& next_normal) const
        {
                ASSERT(dir_to_previous.is_unit());
                const Vector<N, T> next_dir = (next_pos - surface_.point());
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = surface_.pdf(normals_.shading, dir_to_previous, l);
                return solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next_normal);
        }

        [[nodiscard]] Color brdf(const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                ASSERT(v.is_unit());
                ASSERT(l.is_unit());
                return surface_.brdf(normals_.shading, v, l);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !surface_.is_specular();
        }

        [[nodiscard]] T pdf_reversed_over_forward() const
        {
                const auto map = [](const T v)
                {
                        return (v != 0) ? v : 1;
                };
                return map(pdf_reversed_) / map(pdf_forward_);
        }
};

template <std::size_t N, typename T, typename Color>
class Camera final
{
        Vector<N, T> pos_;
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T pdf_forward_ = 1;
        T pdf_reversed_ = 0;

public:
        Camera(const Vector<N, T>& pos, const Color& beta)
                : pos_(pos),
                  beta_(beta)
        {
        }

        [[nodiscard]] const Vector<N, T>& pos() const
        {
                return pos_;
        }

        [[nodiscard]] const std::optional<Vector<N, T>>& normal() const
        {
                return normal_;
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }

        [[nodiscard]] T area_pdf(const T angle_pdf, const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                return solid_angle_pdf_to_area_pdf(pos_, angle_pdf, next_pos, next_normal);
        }

        void set_reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf)
        {
                pdf_reversed_ = next.area_pdf(angle_pdf, pos_, normal_);
        }

        void set_reversed_area_pdf(const T pdf)
        {
                pdf_reversed_ = pdf;
        }

        [[nodiscard]] bool is_connectible() const
        {
                return false;
        }

        [[nodiscard]] T pdf_reversed_over_forward() const
        {
                const auto map = [](const T v)
                {
                        return (v != 0) ? v : 1;
                };
                return map(pdf_reversed_) / map(pdf_forward_);
        }
};

template <std::size_t N, typename T, typename Color>
class Light final
{
        [[nodiscard]] static T compute_pdf_spatial(
                const T light_distribution_pdf,
                const Surface<N, T, Color>& next,
                const LightSource<N, T, Color>* const light,
                const std::optional<Vector<N, T>>& pos,
                const Vector<N, T>& dir)
        {
                const Vector<N, T> l = [&]
                {
                        if (!pos)
                        {
                                return dir;
                        }
                        const Vector<N, T> next_dir = (next.pos() - *pos);
                        const T next_distance = next_dir.norm();
                        return next_dir / next_distance;
                }();
                const T pdf_pos = light->leave_pdf_pos(l);
                return pdf_pos * light_distribution_pdf;
        }

        const LightSource<N, T, Color>* light_;
        std::optional<Vector<N, T>> pos_;
        Vector<N, T> dir_;
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T light_distribution_pdf_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const LightSource<N, T, Color>* const light,
              const LightDistribution<N, T, Color>& distribution,
              const std::optional<Vector<N, T>>& pos,
              const Vector<N, T>& dir,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const T pdf_forward)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  beta_(beta),
                  light_distribution_pdf_(distribution.pdf(light)),
                  pdf_forward_(pdf_forward)
        {
        }

        Light(const LightSource<N, T, Color>* const light,
              const LightDistribution<N, T, Color>& distribution,
              const std::optional<Vector<N, T>>& pos,
              const Vector<N, T>& dir,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const Surface<N, T, Color>& next)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  beta_(beta),
                  light_distribution_pdf_(distribution.pdf(light)),
                  pdf_forward_(compute_pdf_spatial(light_distribution_pdf_, next, light_, pos_, dir_))
        {
        }

        [[nodiscard]] const std::optional<Vector<N, T>>& pos() const
        {
                return pos_;
        }

        [[nodiscard]] const Vector<N, T>& dir() const
        {
                ASSERT(!pos_);
                return dir_;
        }

        [[nodiscard]] const std::optional<Vector<N, T>>& normal() const
        {
                return normal_;
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }

        [[nodiscard]] T area_pdf(const T angle_pdf, const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                if (pos_)
                {
                        return solid_angle_pdf_to_area_pdf(*pos_, angle_pdf, next_pos, next_normal);
                }
                return pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next_normal);
        }

        [[nodiscard]] T area_pdf(const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                if (!pos_)
                {
                        return pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next_normal);
                }
                const Vector<N, T> next_dir = (next_pos - *pos_);
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = light_->leave_pdf_dir(l);
                return solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next_normal);
        }

        void set_reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf)
        {
                if (!pos_)
                {
                        pdf_reversed_ = 0;
                        return;
                }
                pdf_reversed_ = next.area_pdf(angle_pdf, *pos_, normal_);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !light_->is_delta();
        }

        [[nodiscard]] T pdf_reversed_over_forward() const
        {
                const auto map = [](const T v)
                {
                        return (v != 0) ? v : 1;
                };
                return map(pdf_reversed_) / map(pdf_forward_);
        }
};

template <std::size_t N, typename T, typename Color>
class InfiniteLight final
{
        [[nodiscard]] T angle_pdf_reversed(const Vector<N, T>& prev_pos) const
        {
                T res = 0;
                T sum = 0;
                for (const LightSource<N, T, Color>* const light : scene_->light_sources())
                {
                        if (!light->is_infinite_area())
                        {
                                continue;
                        }
                        const LightSourceArriveInfo<T, Color> info = light->arrive_info(prev_pos, ray_to_light_.dir());
                        const T distribution_pdf = light_distribution_->pdf(light);
                        res += info.pdf * distribution_pdf;
                        sum += distribution_pdf;
                }
                if (sum > 0)
                {
                        res /= sum;
                }
                return 0;
        }

        const Scene<N, T, Color>* scene_;
        LightDistribution<N, T, Color>* light_distribution_;
        Ray<N, T> ray_to_light_;
        Color beta_;
        T angle_pdf_forward_;
        T angle_pdf_reversed_;

public:
        InfiniteLight(
                const Scene<N, T, Color>* const scene,
                LightDistribution<N, T, Color>* const light_distribution,
                const Ray<N, T>& ray_to_light,
                const Color& beta,
                const T angle_pdf_forward)
                : scene_(scene),
                  light_distribution_(light_distribution),
                  ray_to_light_(ray_to_light),
                  beta_(beta),
                  angle_pdf_forward_(angle_pdf_forward),
                  angle_pdf_reversed_(angle_pdf_reversed(ray_to_light.org()))
        {
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }

        [[nodiscard]] const Ray<N, T>& ray_to_light() const
        {
                return ray_to_light_;
        }

        template <typename Normal>
        [[nodiscard]] T area_pdf(const Normal& next_normal) const
        {
                const Vector<N, T> dir = -ray_to_light_.dir();
                T res = 0;
                T sum = 0;
                for (const LightSource<N, T, Color>* const light : scene_->light_sources())
                {
                        if (!light->is_infinite_area())
                        {
                                continue;
                        }
                        const T pdf = light->leave_pdf_pos(dir);
                        const T distribution_pdf = light_distribution_->pdf(light);
                        res += pdf * distribution_pdf;
                        sum += distribution_pdf;
                }
                if (sum > 0)
                {
                        res /= sum;
                        return pos_pdf_to_area_pdf(res, dir, next_normal);
                }
                return 0;
        }

        [[nodiscard]] T pdf_reversed_over_forward() const
        {
                const auto map = [](const T v)
                {
                        return (v != 0) ? v : 1;
                };
                return map(angle_pdf_reversed_) / map(angle_pdf_forward_);
        }
};

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Surface<N, T, Color>, Camera<N, T, Color>, Light<N, T, Color>, InfiniteLight<N, T, Color>>;
}
