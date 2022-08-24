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

#include "../objects.h"

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

template <typename Vertex, typename Normal, std::size_t N, typename T>
T compute_pdf(const Vertex& prev, const T angle_pdf, const Vector<N, T>& next_pos, const Normal& next_normal)
{
        return std::visit(
                [&](const auto& v)
                {
                        return solid_angle_pdf_to_area_pdf(v.pos(), angle_pdf, next_pos, next_normal);
                },
                prev);
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

        const Vector<N, T>& pos() const
        {
                return surface_.point();
        }

        template <typename Vertex>
        void set_forward_pdf(const Vertex& prev, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::compute_pdf(prev, forward_angle_pdf, surface_.point(), normal_);
        }

        template <typename Vertex>
        void set_reversed_pdf(const Vertex& next, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ = impl::compute_pdf(next, reversed_angle_pdf, surface_.point(), normal_);
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

        const Vector<N, T>& pos() const
        {
                return pos_;
        }

        template <typename Vertex>
        void set_forward_pdf(const Vertex& prev, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::compute_pdf(prev, forward_angle_pdf, pos_, normal_);
        }

        template <typename Vertex>
        void set_reversed_pdf(const Vertex& next, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ = impl::compute_pdf(next, reversed_angle_pdf, pos_, normal_);
        }
};

template <std::size_t N, typename T, typename Color>
class Light final
{
        Vector<N, T> pos_;
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const Vector<N, T>& pos,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const T pdf_forward)
                : pos_(pos),
                  normal_(normal),
                  beta_(beta),
                  pdf_forward_(pdf_forward)
        {
        }

        const Vector<N, T>& pos() const
        {
                return pos_;
        }

        template <typename Vertex>
        void set_forward_pdf(const Vertex& prev, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::compute_pdf(prev, forward_angle_pdf, pos_, normal_);
        }

        template <typename Vertex>
        void set_reversed_pdf(const Vertex& next, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ = impl::compute_pdf(next, reversed_angle_pdf, pos_, normal_);
        }
};

template <typename Vertex, typename T>
void set_forward_pdf(const Vertex& prev, Vertex* const next, const T pdf_forward)
{
        std::visit(
                [&](auto& v)
                {
                        v.set_forward_pdf(prev, pdf_forward);
                },
                *next);
}

template <typename Vertex, typename T>
void set_reversed_pdf(Vertex* const prev, const Vertex& next, const T pdf_reversed)
{
        std::visit(
                [&](auto& v)
                {
                        v.set_reversed_pdf(next, pdf_reversed);
                },
                *prev);
}

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Camera<N, T, Color>, Light<N, T, Color>, Surface<N, T, Color>>;
}
