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

#include "light_distribution.h"

#include "../objects.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>
#include <src/sampling/pdf.h>

#include <cmath>
#include <variant>

namespace ns::painter::integrators
{
namespace bpt_vertex_implementation
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
}

template <std::size_t N, typename T, typename Color>
class Surface final
{
        SurfaceIntersection<N, T, Color> surface_;
        Vector<N, T> normal_;
        Color beta_;
        T pdf_forward_ = 0;
        T pdf_reversed_ = 0;

public:
        Surface(const SurfaceIntersection<N, T, Color>& surface, const Vector<N, T>& normal, const Color& beta)
                : surface_(surface),
                  normal_(normal),
                  beta_(beta)
        {
        }

        [[nodiscard]] const Vector<N, T>& pos() const
        {
                return surface_.point();
        }

        [[nodiscard]] const Vector<N, T>& normal() const
        {
                return normal_;
        }

        template <typename Prev>
        void set_forward_pdf(const Prev& prev, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ =
                        impl::solid_angle_pdf_to_area_pdf(prev.pos(), forward_angle_pdf, surface_.point(), normal_);
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ =
                        impl::solid_angle_pdf_to_area_pdf(next.pos(), reversed_angle_pdf, surface_.point(), normal_);
        }

        template <typename Prev, typename Next>
        [[nodiscard]] T compute_pdf(const Prev& prev, const Next& next) const
        {
                namespace impl = bpt_vertex_implementation;
                const Vector<N, T> v = (prev.pos() - surface_.point()).normalized();
                const Vector<N, T> next_dir = (next.pos() - surface_.point());
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = surface_.pdf(normal_, v, l);
                return impl::solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next.normal());
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

        template <typename Prev>
        void set_forward_pdf(const Prev& prev, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::solid_angle_pdf_to_area_pdf(prev.pos(), forward_angle_pdf, pos_, normal_);
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
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

        [[nodiscard]] const Vector<N, T>& pos() const
        {
                return pos_;
        }

        [[nodiscard]] const std::optional<Vector<N, T>>& normal() const
        {
                return normal_;
        }

        template <typename Prev>
        void set_forward_pdf(const Prev& prev, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::solid_angle_pdf_to_area_pdf(prev.pos(), forward_angle_pdf, pos_, normal_);
        }

        template <typename Next>
        void set_reversed_pdf(const Next& next, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(next.pos(), reversed_angle_pdf, pos_, normal_);
        }

        template <typename Next>
        [[nodiscard]] T compute_pdf(const Next& next) const
        {
                namespace impl = bpt_vertex_implementation;
                const Vector<N, T> next_dir = (next.pos() - pos_);
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = light_->emit_pdf_dir(pos_, l);
                return impl::solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next.normal());
        }

        template <typename Next>
        [[nodiscard]] T compute_pdf_spatial(const LightDistribution<N, T, Color>& distribution, const Next& next) const
        {
                const Vector<N, T> next_dir = (next.pos() - pos_);
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf_pos = light_->emit_pdf_pos(pos_, l);
                const T distribution_pdf = distribution.pdf(light_);
                return pdf_pos * distribution_pdf;
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !light_->is_delta();
        }
};

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
void set_forward_pdf(
        const std::variant<Vertex<N, T, Color>...>& prev,
        std::variant<Vertex<N, T, Color>...>* const next,
        const T pdf_forward)
{
        std::visit(
                [&](auto& v_next)
                {
                        std::visit(
                                [&](const auto& v_prev)
                                {
                                        v_next.set_forward_pdf(v_prev, pdf_forward);
                                },
                                prev);
                },
                *next);
}

template <std::size_t N, typename T, typename Color, template <std::size_t, typename, typename> typename... Vertex>
void set_reversed_pdf(
        std::variant<Vertex<N, T, Color>...>* const prev,
        const std::variant<Vertex<N, T, Color>...>& next,
        const T pdf_reversed)
{
        std::visit(
                [&](auto& v_prev)
                {
                        std::visit(
                                [&](const auto& v_next)
                                {
                                        v_prev.set_reversed_pdf(v_next, pdf_reversed);
                                },
                                next);
                },
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

template <typename Vertex>
[[nodiscard]] decltype(auto) is_connectible(const Vertex& vertex)
{
        return std::visit(
                [&](const auto& v)
                {
                        return v.is_connectible();
                },
                vertex);
}

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Camera<N, T, Color>, Light<N, T, Color>, Surface<N, T, Color>>;
}
