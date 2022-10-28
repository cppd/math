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
                ASSERT(dir_to_prev_.is_unit());
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

        [[nodiscard]] bool is_light() const
        {
                return surface_.light_source() != nullptr;
        }

        [[nodiscard]] decltype(auto) light_radiance() const
        {
                ASSERT(surface_.light_source() != nullptr);
                return surface_.light_source()->leave_radiance(dir_to_prev_);
        }

        [[nodiscard]] T light_area_pdf(const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                ASSERT(surface_.light_source() != nullptr);
                const Vector<N, T> l_dir = (next_pos - surface_.point());
                const T l_distance = l_dir.norm();
                const Vector<N, T> l = l_dir / l_distance;
                const T pdf = surface_.light_source()->leave_pdf_dir(l);
                return solid_angle_pdf_to_area_pdf(pdf, l, l_distance, next_normal);
        }

        [[nodiscard]] T light_area_origin_pdf() const
        {
                ASSERT(surface_.light_source() != nullptr);
                return surface_.light_source()->leave_pdf_pos(dir_to_prev_);
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

        [[nodiscard]] T reversed_pdf(const Vector<N, T>& v, const Surface<N, T, Color>& next) const
        {
                ASSERT(v.is_unit());
                const Vector<N, T> l_dir = (surface_.point() - next.pos());
                const T l_distance = l_dir.norm();
                const Vector<N, T> l = l_dir / l_distance;
                const T pdf = next.angle_pdf(v, l);
                return solid_angle_pdf_to_area_pdf(pdf, l, l_distance, normals_.shading);
        }

        void set_reversed_area_pdf(const T pdf)
        {
                pdf_reversed_ = pdf;
        }

        [[nodiscard]] T angle_pdf(const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                ASSERT(v.is_unit());
                ASSERT(l.is_unit());
                return surface_.pdf(normals_.shading, v, l);
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

        [[nodiscard]] T pdf_reversed() const
        {
                return pdf_reversed_;
        }

        [[nodiscard]] T pdf_forward() const
        {
                return pdf_forward_;
        }
};

template <std::size_t N, typename T, typename Color>
class Camera final
{
        Vector<N, T> dir_to_camera_;

public:
        explicit Camera(const Vector<N, T>& dir)
                : dir_to_camera_(-dir)
        {
                ASSERT(dir_to_camera_.is_unit());
        }

        [[nodiscard]] const Vector<N, T>& dir_to_camera() const
        {
                return dir_to_camera_;
        }

        [[nodiscard]] T area_pdf(
                [[maybe_unused]] const T angle_pdf,
                const Vector<N, T>& /*next_pos*/,
                const Vector<N, T>& /*next_normal*/) const
        {
                ASSERT(angle_pdf == 1);
                return 1;
        }

        [[nodiscard]] bool is_connectible() const
        {
                return true;
        }
};

template <std::size_t N, typename T, typename Color>
class Light final
{
        const LightSource<N, T, Color>* light_;
        std::optional<Vector<N, T>> pos_;
        Vector<N, T> dir_;
        std::optional<Vector<N, T>> normal_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const LightSource<N, T, Color>* const light,
              const std::optional<Vector<N, T>>& pos,
              const Vector<N, T>& dir,
              const std::optional<Vector<N, T>>& normal,
              const T pdf_distribution,
              const T pdf_pos,
              const T pdf_dir)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  pdf_forward_(pdf_distribution * (!light->is_infinite_area() ? pdf_pos : pdf_dir))
        {
        }

        Light(const LightSource<N, T, Color>* const light,
              const T pdf_distribution,
              const T pdf_dir,
              const std::optional<Vector<N, T>>& pos,
              const Vector<N, T>& dir,
              const std::optional<Vector<N, T>>& normal,
              const Surface<N, T, Color>& next)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  pdf_forward_(
                          pdf_distribution
                          * (!light->is_infinite_area()
                                     ? light_->leave_pdf_pos(!pos_ ? dir_ : (next.pos() - *pos_).normalized())
                                     : pdf_dir))
        {
        }

        [[nodiscard]] const std::optional<Vector<N, T>>& pos() const
        {
                return pos_;
        }

        [[nodiscard]] Vector<N, T> dir_to_light(const Vector<N, T>& point) const
        {
                if (pos_)
                {
                        return (*pos_ - point).normalized();
                }
                return -dir_;
        }

        [[nodiscard]] const std::optional<Vector<N, T>>& normal() const
        {
                return normal_;
        }

        [[nodiscard]] T area_pdf(const T angle_pdf, const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                if (!pos_)
                {
                        return pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next_normal);
                }
                return solid_angle_pdf_to_area_pdf(*pos_, angle_pdf, next_pos, next_normal);
        }

        [[nodiscard]] T area_pdf(const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                if (!pos_)
                {
                        return pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next_normal);
                }
                const Vector<N, T> l_dir = (next_pos - *pos_);
                const T l_distance = l_dir.norm();
                const Vector<N, T> l = l_dir / l_distance;
                const T pdf = light_->leave_pdf_dir(l);
                return solid_angle_pdf_to_area_pdf(pdf, l, l_distance, next_normal);
        }

        void set_reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf)
        {
                pdf_reversed_ = this->reversed_pdf(next, angle_pdf);
        }

        [[nodiscard]] T reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf) const
        {
                if (!pos_)
                {
                        if (light_->is_infinite_area())
                        {
                                return angle_pdf;
                        }
                        return 0;
                }
                ASSERT(!light_->is_infinite_area());
                return next.area_pdf(angle_pdf, *pos_, normal_);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !light_->is_delta();
        }

        [[nodiscard]] T pdf_reversed() const
        {
                return pdf_reversed_;
        }

        [[nodiscard]] T pdf_forward() const
        {
                return pdf_forward_;
        }
};

template <std::size_t N, typename T, typename Color>
class InfiniteLight final
{
        [[nodiscard]] static T angle_pdf_origin(
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

        template <typename Normal>
        [[nodiscard]] static T area_pdf(
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
                  angle_pdf_origin_(angle_pdf_origin(*scene, *light_distribution, ray_to_light))
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
                return area_pdf(*scene_, *light_distribution_, dir_, next_normal);
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

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Surface<N, T, Color>, Camera<N, T, Color>, Light<N, T, Color>, InfiniteLight<N, T, Color>>;
}
