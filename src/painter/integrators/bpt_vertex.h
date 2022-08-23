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
}

template <std::size_t N, typename T, typename Color>
class VertexBase
{
        Vector<N, T> pos_;

protected:
        ~VertexBase() = default;

public:
        explicit VertexBase(const Vector<N, T>& pos)
                : pos_(pos)
        {
        }

        [[nodiscard]] const Vector<N, T>& pos() const
        {
                return pos_;
        }
};

template <std::size_t N, typename T, typename Color>
class Surface final : public VertexBase<N, T, Color>
{
        SurfaceIntersection<N, T, Color> surface_;
        Vector<N, T> normal_;
        Color beta_;
        T pdf_forward_ = 0;
        T pdf_reversed_ = 0;

public:
        Surface(const SurfaceIntersection<N, T, Color>& surface, const Vector<N, T>& normal, const Color& beta)
                : VertexBase<N, T, Color>(surface.point()),
                  surface_(surface),
                  normal_(normal),
                  beta_(beta)
        {
        }

        void set_forward_pdf(const Vector<N, T>& prev_pos, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ =
                        impl::solid_angle_pdf_to_area_pdf(prev_pos, forward_angle_pdf, surface_.point(), normal_);
        }

        void set_reversed_pdf(const Vector<N, T>& next_pos, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ =
                        impl::solid_angle_pdf_to_area_pdf(next_pos, reversed_angle_pdf, surface_.point(), normal_);
        }
};

template <std::size_t N, typename T, typename Color>
class Camera final : public VertexBase<N, T, Color>
{
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T pdf_forward_ = 1;
        T pdf_reversed_ = 0;

public:
        Camera(const Vector<N, T>& pos, const Color& beta)
                : VertexBase<N, T, Color>(pos),
                  beta_(beta)
        {
        }

        void set_forward_pdf(const Vector<N, T>& prev_pos, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::solid_angle_pdf_to_area_pdf(prev_pos, forward_angle_pdf, this->pos(), normal_);
        }

        void set_reversed_pdf(const Vector<N, T>& next_pos, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(next_pos, reversed_angle_pdf, this->pos(), normal_);
        }
};

template <std::size_t N, typename T, typename Color>
class Light final : public VertexBase<N, T, Color>
{
        std::optional<Vector<N, T>> normal_;
        Color beta_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const Vector<N, T>& pos,
              const std::optional<Vector<N, T>>& normal,
              const Color& beta,
              const T pdf_forward)
                : VertexBase<N, T, Color>(pos),
                  normal_(normal),
                  beta_(beta),
                  pdf_forward_(pdf_forward)
        {
        }

        void set_forward_pdf(const Vector<N, T>& prev_pos, const T forward_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_forward_ = impl::solid_angle_pdf_to_area_pdf(prev_pos, forward_angle_pdf, this->pos(), normal_);
        }

        void set_reversed_pdf(const Vector<N, T>& next_pos, const T reversed_angle_pdf)
        {
                namespace impl = bpt_vertex_implementation;
                pdf_reversed_ = impl::solid_angle_pdf_to_area_pdf(next_pos, reversed_angle_pdf, this->pos(), normal_);
        }
};

template <std::size_t N, typename T, typename Color>
using Vertex = std::variant<Camera<N, T, Color>, Light<N, T, Color>, Surface<N, T, Color>>;

template <std::size_t N, typename T, typename Color>
[[nodiscard]] const Vector<N, T>& vertex_pos(const Vertex<N, T, Color>& vertex)
{
        return std::visit(
                [&](const auto& v) -> const Vector<N, T>&
                {
                        return v.pos();
                },
                vertex);
}

template <std::size_t N, typename T, typename Color>
void set_forward_pdf(const Vertex<N, T, Color>& prev, Vertex<N, T, Color>* const next, const T pdf_forward)
{
        std::visit(
                [&](auto& v)
                {
                        v.set_forward_pdf(vertex_pos(prev), pdf_forward);
                },
                *next);
}

template <std::size_t N, typename T, typename Color>
void set_reversed_pdf(Vertex<N, T, Color>* const prev, const Vertex<N, T, Color>& next, const T pdf_reversed)
{
        std::visit(
                [&](auto& v)
                {
                        v.set_reversed_pdf(vertex_pos(next), pdf_reversed);
                },
                *prev);
}
}
