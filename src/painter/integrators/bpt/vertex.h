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
        T pdf_forward_ = 0;
        T pdf_reversed_ = 0;

public:
        Surface(const SurfaceIntersection<N, T, Color>& surface, const Normals<N, T>& normals, const Color& beta)
                : surface_(surface),
                  normals_(normals),
                  beta_(beta)
        {
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

        template <typename Prev>
        void set_forward_pdf(const Prev& prev, const T forward_angle_pdf)
        {
                namespace impl = vertex_implementation;
                pdf_forward_ = impl::solid_angle_pdf_to_area_pdf(
                        prev.pos(), forward_angle_pdf, surface_.point(), normals_.shading);
        }

        void set_forward_pos_pdf(const Vector<N, T>& dir, const T pos_pdf)
        {
                namespace impl = vertex_implementation;
                pdf_forward_ = impl::pos_pdf_to_area_pdf(pos_pdf, dir, normals_.shading);
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(
                        next.pos(), reversed_angle_pdf, surface_.point(), normals_.shading);
        }

        template <typename Prev, typename Next>
        [[nodiscard]] T compute_pdf(const Prev& prev, const Next& next) const
        {
                namespace impl = vertex_implementation;
                const Vector<N, T> v = (prev.pos() - surface_.point()).normalized();
                const Vector<N, T> next_dir = (next.pos() - surface_.point());
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = surface_.pdf(normals_.shading, v, l);
                return impl::solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next.normal());
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
                const Vector<N, T>& pos)
        {
                const Vector<N, T> next_dir = (next.pos() - pos);
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf_pos = light->leave_pdf_pos(pos, l);
                const T distribution_pdf = distribution.pdf(light);
                return pdf_pos * distribution_pdf;
        }

        const LightSource<N, T, Color>* light_;
        Vector<N, T> pos_;
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const LightSource<N, T, Color>* const light,
              const Vector<N, T>& pos,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const T pdf_forward)
                : light_(light),
                  pos_(pos),
                  normal_(normal),
                  beta_(beta),
                  pdf_forward_(pdf_forward)
        {
        }

        template <typename Next>
        Light(const LightSource<N, T, Color>* const light,
              const Vector<N, T>& pos,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const LightDistribution<N, T, Color>& distribution,
              const Next& next)
                : light_(light),
                  pos_(pos),
                  normal_(normal),
                  beta_(beta),
                  pdf_forward_(compute_pdf_spatial(distribution, next, light_, pos_))
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

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(next.pos(), reversed_angle_pdf, pos_, normal_);
        }

        template <typename Next>
        [[nodiscard]] T compute_pdf(const Next& next) const
        {
                namespace impl = vertex_implementation;
                const Vector<N, T> next_dir = (next.pos() - pos_);
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = light_->leave_pdf_dir(pos_, l);
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
        Color beta_;
        T pdf_forward_;

public:
        InfiniteLight(const Color& beta, const T pdf_forward)
                : beta_(beta),
                  pdf_forward_(pdf_forward)
        {
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }
};

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
void set_forward_pdf(
        const std::variant<Vertex<N, T, Color>...>& prev,
        std::variant<Vertex<N, T, Color>...>* const next,
        const T pdf_forward)
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
                                std::get<Surface<N, T, Color>>(*next).set_forward_pdf(v_prev, pdf_forward);
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
[[nodiscard]] decltype(auto) compute_pdf(
        const std::variant<Vertex<N, T, Color>...>& vertex,
        const std::variant<Vertex<N, T, Color>...>& prev,
        const std::variant<Vertex<N, T, Color>...>& next)
{
        return std::visit(
                [&](const auto& v_prev)
                {
                        return std::visit(
                                [&](const auto& v_next)
                                {
                                        ASSERT((std::holds_alternative<Surface<N, T, Color>>(vertex)));
                                        return std::get<Surface<N, T, Color>>(vertex).compute_pdf(v_prev, v_next);
                                },
                                next);
                },
                prev);
}

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Camera<N, T, Color>, InfiniteLight<N, T, Color>, Light<N, T, Color>, Surface<N, T, Color>>;
}
