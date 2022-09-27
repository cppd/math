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

#include "../../objects.h"
#include "../com/normals.h"

#include <src/com/error.h>
#include <src/com/variant.h>
#include <src/numerical/vector.h>
#include <src/sampling/pdf.h>

#include <cmath>
#include <variant>

namespace ns::painter::integrators::bpt
{
namespace vertex_implementation
{
template <std::size_t N, typename T, typename Normal>
[[nodiscard]] T solid_angle_pdf_to_area_pdf(
        const Vector<N, T>& prev_pos,
        const T angle_pdf,
        const Vector<N, T>& next_pos,
        const Normal& next_normal)
{
        const Vector<N, T> v = prev_pos - next_pos;
        const T distance = v.norm();
        const T cosine = [&]
        {
                if constexpr (requires { dot(v, *next_normal); })
                {
                        return next_normal ? (std::abs(dot(v, *next_normal)) / distance) : 1;
                }
                else
                {
                        return std::abs(dot(v, next_normal)) / distance;
                }
        }();
        return sampling::solid_angle_pdf_to_area_pdf<N, T>(angle_pdf, cosine, distance);
}

template <std::size_t N, typename T, typename Normal>
[[nodiscard]] T solid_angle_pdf_to_area_pdf(
        const T angle_pdf,
        const Vector<N, T>& next_dir,
        const T next_distance,
        const Normal& next_normal)
{
        ASSERT(next_dir.is_unit());
        const T cosine = [&]
        {
                if constexpr (requires { dot(next_dir, *next_normal); })
                {
                        return next_normal ? std::abs(dot(next_dir, *next_normal)) : 1;
                }
                else
                {
                        return std::abs(dot(next_dir, next_normal));
                }
        }();
        return sampling::solid_angle_pdf_to_area_pdf<N, T>(angle_pdf, cosine, next_distance);
}

template <std::size_t N, typename T>
[[nodiscard]] T pos_pdf_to_area_pdf(const T pos_pdf, const Vector<N, T>& dir, const Vector<N, T>& next_normal)
{
        const T cosine = std::abs(dot(dir, next_normal));
        return pos_pdf * cosine;
}
}

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

        [[nodiscard]] T area_pdf(const T angle_pdf, const Surface<N, T, Color>& next) const
        {
                namespace impl = vertex_implementation;
                return impl::solid_angle_pdf_to_area_pdf(surface_.point(), angle_pdf, next.pos(), next.normal());
        }

        void set_forward_pdf(const T forward_pdf)
        {
                pdf_forward_ = forward_pdf;
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(
                        next.pos(), reversed_angle_pdf, surface_.point(), normals_.shading);
        }

        [[nodiscard]] T pdf(const Vector<N, T>& v, const Vector<N, T>& l) const
        {
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

        [[nodiscard]] T area_pdf(const T angle_pdf, const Surface<N, T, Color>& next) const
        {
                namespace impl = vertex_implementation;
                return impl::solid_angle_pdf_to_area_pdf(pos_, angle_pdf, next.pos(), next.normal());
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(next.pos(), reversed_angle_pdf, pos_, normal_);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return false;
        }
};

template <std::size_t N, typename T, typename Color>
class Light final
{
        template <typename Next>
        [[nodiscard]] static T compute_pdf_spatial(
                const LightDistribution<N, T, Color>& distribution,
                const Next& next,
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
                const T distribution_pdf = distribution.pdf(light);
                return pdf_pos * distribution_pdf;
        }

        const LightSource<N, T, Color>* light_;
        std::optional<Vector<N, T>> pos_;
        Vector<N, T> dir_;
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const LightSource<N, T, Color>* const light,
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
                  pdf_forward_(pdf_forward)
        {
        }

        template <typename Next>
        Light(const LightSource<N, T, Color>* const light,
              const std::optional<Vector<N, T>>& pos,
              const Vector<N, T>& dir,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const LightDistribution<N, T, Color>& distribution,
              const Next& next)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  beta_(beta),
                  pdf_forward_(compute_pdf_spatial(distribution, next, light_, pos_, dir_))
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

        [[nodiscard]] T area_pdf(const T angle_pdf, const Surface<N, T, Color>& next) const
        {
                namespace impl = vertex_implementation;
                if (pos_)
                {
                        return impl::solid_angle_pdf_to_area_pdf(*pos_, angle_pdf, next.pos(), next.normal());
                }
                return impl::pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next.normal());
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = vertex_implementation;
                if (!pos_)
                {
                        pdf_reversed_ = 0;
                        return;
                }
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(next.pos(), reversed_angle_pdf, *pos_, normal_);
        }

        template <typename Next>
        [[nodiscard]] T compute_pdf(const Next& next) const
        {
                namespace impl = vertex_implementation;
                if (!pos_)
                {
                        return impl::pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next.normal());
                }
                const Vector<N, T> next_dir = (next.pos() - *pos_);
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = light_->leave_pdf_dir(l);
                return impl::solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next.normal());
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !light_->is_delta();
        }
};

template <std::size_t N, typename T, typename Color>
class InfiniteLight final
{
        Ray<N, T> ray_to_light_;
        Color beta_;
        T pdf_forward_;

public:
        InfiniteLight(const Ray<N, T>& ray_to_light, const Color& beta, const T pdf_forward)
                : ray_to_light_(ray_to_light),
                  beta_(beta),
                  pdf_forward_(pdf_forward)
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
};

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
void set_forward_pdf(
        const std::variant<Vertex<N, T, Color>...>& prev,
        std::variant<Vertex<N, T, Color>...>* const next,
        const T angle_pdf)
{
        std::visit(
                Visitors{
                        [&](const InfiniteLight<N, T, Color>&)
                        {
                                error_fatal("Previous vertex is an infinite light");
                        },
                        [&](const auto& v_prev)
                        {
                                ASSERT((std::holds_alternative<Surface<N, T, Color>>(*next)));
                                auto& surface = std::get<Surface<N, T, Color>>(*next);
                                surface.set_forward_pdf(v_prev.area_pdf(angle_pdf, surface));
                        }},
                prev);
}

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
void set_reversed_pdf(
        std::variant<Vertex<N, T, Color>...>* const prev,
        const std::variant<Vertex<N, T, Color>...>& next,
        const T pdf_reversed)
{
        std::visit(
                Visitors{
                        [&](InfiniteLight<N, T, Color>&)
                        {
                                error_fatal("Previous vertex is an infinite light");
                        },
                        [&](auto& v_prev)
                        {
                                ASSERT((std::holds_alternative<Surface<N, T, Color>>(next)));
                                v_prev.set_reversed_pdf(std::get<Surface<N, T, Color>>(next), pdf_reversed);
                        }},
                *prev);
}

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
[[nodiscard]] T compute_pdf(
        const std::variant<Vertex<N, T, Color>...>& vertex,
        const std::variant<Vertex<N, T, Color>...>& next)
{
        return std::visit(
                Visitors{
                        [&](const Light<N, T, Color>& l) -> T
                        {
                                ASSERT((std::holds_alternative<Surface<N, T, Color>>(next)));
                                return l.compute_pdf(std::get<Surface<N, T, Color>>(next));
                        },
                        [&](const auto&) -> T
                        {
                                error_fatal("Vertex is not a light");
                        },
                },
                vertex);
}

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
[[nodiscard]] T compute_pdf(
        const std::variant<Vertex<N, T, Color>...>& vertex,
        const std::variant<Vertex<N, T, Color>...>& prev,
        const std::variant<Vertex<N, T, Color>...>& next)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(vertex)));
        const auto& surface = std::get<Surface<N, T, Color>>(vertex);

        const Vector<N, T> v = std::visit(
                Visitors{
                        [&](const InfiniteLight<N, T, Color>&) -> Vector<N, T>
                        {
                                error_fatal("Previous vertex is an infinite light");
                        },
                        [&](const Camera<N, T, Color>&) -> Vector<N, T>
                        {
                                error_fatal("Previous vertex is a camera");
                        },
                        [&](const Surface<N, T, Color>& v_prev) -> Vector<N, T>
                        {
                                return (v_prev.pos() - surface.pos()).normalized();
                        },
                        [&](const Light<N, T, Color>& v_prev) -> Vector<N, T>
                        {
                                if (v_prev.pos())
                                {
                                        return (*v_prev.pos() - surface.pos()).normalized();
                                }
                                return -v_prev.dir();
                        }},
                prev);

        const auto compute_pos_pdf = [&](const auto& next_pos, const auto& next_normal)
        {
                namespace impl = vertex_implementation;

                const Vector<N, T> next_dir = (next_pos - surface.pos());
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = surface.pdf(v, l);
                return impl::solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next_normal);
        };

        return std::visit(
                Visitors{
                        [&](const InfiniteLight<N, T, Color>& v_next) -> T
                        {
                                const Vector<N, T> l = v_next.ray_to_light().dir();
                                return surface.pdf(v, l);
                        },
                        [&](const Camera<N, T, Color>&) -> T
                        {
                                error_fatal("Next vertex is a camera");
                        },
                        [&](const Surface<N, T, Color>& v_next) -> T
                        {
                                return compute_pos_pdf(v_next.pos(), v_next.normal());
                        },
                        [&](const Light<N, T, Color>& v_next) -> T
                        {
                                if (!v_next.pos())
                                {
                                        return 0;
                                }
                                return compute_pos_pdf(*v_next.pos(), v_next.normal());
                        }},
                next);
}

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Camera<N, T, Color>, InfiniteLight<N, T, Color>, Light<N, T, Color>, Surface<N, T, Color>>;
}
