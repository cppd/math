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

#include "probability_density.h"
#include "vertex.h"

#include <src/com/error.h>
#include <src/com/variant.h>
#include <src/numerical/vector.h>

#include <variant>

namespace ns::painter::integrators::bpt
{
template <std::size_t N, typename T, typename Color>
void set_forward_pdf(
        const Vertex<N, T, Color>& prev_vertex,
        Surface<N, T, Color>* const next_surface,
        const T angle_pdf)
{
        std::visit(
                Visitors{
                        [&](const InfiniteLight<N, T, Color>&)
                        {
                                error_fatal("Previous vertex is an infinite light");
                        },
                        [&](const auto& prev)
                        {
                                next_surface->set_forward_pdf(prev.area_pdf(angle_pdf, *next_surface));
                        }},
                prev_vertex);
}

template <std::size_t N, typename T, typename Color>
void set_reversed_pdf(
        Vertex<N, T, Color>* const prev_vertex,
        const Surface<N, T, Color>& next_surface,
        const T pdf_reversed)
{
        std::visit(
                Visitors{
                        [&](InfiniteLight<N, T, Color>&)
                        {
                                error_fatal("Previous vertex is an infinite light");
                        },
                        [&](auto& prev)
                        {
                                prev.set_reversed_pdf(next_surface, pdf_reversed);
                        }},
                *prev_vertex);
}

template <std::size_t N, typename T, typename Color>
void set_reversed_pdf(Vertex<N, T, Color>* const prev_vertex, const InfiniteLight<N, T, Color>& next_light)
{
        std::visit(
                Visitors{
                        [&](Surface<N, T, Color>& prev)
                        {
                                prev.set_reversed_area_pdf(next_light.compute_pdf(prev));
                        },
                        [&](Camera<N, T, Color>& prev)
                        {
                                prev.set_reversed_area_pdf(next_light.compute_pdf(prev));
                        },
                        [](const auto&)
                        {
                                error_fatal("Previous vertex is not a surface or a camera");
                        }},
                *prev_vertex);
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T compute_pdf(const Vertex<N, T, Color>& vertex, const Vertex<N, T, Color>& next)
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

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T compute_pdf(
        const Vertex<N, T, Color>& vertex,
        const Vertex<N, T, Color>& prev,
        const Vertex<N, T, Color>& next)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(vertex)));
        const auto& surface = std::get<Surface<N, T, Color>>(vertex);

        const Vector<N, T> v = std::visit(
                Visitors{
                        [&](const InfiniteLight<N, T, Color>&) -> Vector<N, T>
                        {
                                error_fatal("Previous vertex is an infinite light");
                        },
                        [&](const Camera<N, T, Color>& v_prev) -> Vector<N, T>
                        {
                                return (v_prev.pos() - surface.pos()).normalized();
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
                const Vector<N, T> next_dir = (next_pos - surface.pos());
                const T next_distance = next_dir.norm();
                const Vector<N, T> l = next_dir / next_distance;
                const T pdf = surface.pdf(v, l);
                return solid_angle_pdf_to_area_pdf(pdf, l, next_distance, next_normal);
        };

        return std::visit(
                Visitors{
                        [&](const InfiniteLight<N, T, Color>&) -> T
                        {
                                error_fatal("Next vertex is an infinite light");
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
}
