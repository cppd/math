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
                        [&](const Surface<N, T, Color>& prev)
                        {
                                next_surface->set_forward_pdf(prev, angle_pdf);
                        },
                        [&](const Camera<N, T, Color>& prev)
                        {
                                next_surface->set_forward_pdf(prev, angle_pdf);
                        },
                        [&](const Light<N, T, Color>& prev)
                        {
                                next_surface->set_forward_pdf(prev, angle_pdf);
                        },
                        [](const InfiniteLight<N, T, Color>&)
                        {
                                error("Previous vertex is an infinite light");
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
                        [&](Surface<N, T, Color>& prev)
                        {
                                prev.set_reversed_pdf(next_surface, pdf_reversed);
                        },
                        [](Camera<N, T, Color>&)
                        {
                        },
                        [&](Light<N, T, Color>& prev)
                        {
                                prev.set_reversed_pdf(next_surface, pdf_reversed);
                        },
                        [](const InfiniteLight<N, T, Color>&)
                        {
                                error("Previous vertex is an infinite light");
                        }},
                *prev_vertex);
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T compute_light_pdf(const Vertex<N, T, Color>& light_vertex, const Vertex<N, T, Color>& next_vertex)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(next_vertex)));
        const auto& surface = std::get<Surface<N, T, Color>>(next_vertex);

        return std::visit(
                Visitors{
                        [](const Surface<N, T, Color>&) -> T
                        {
                                error("Light vertex is a surface");
                        },
                        [](const Camera<N, T, Color>&) -> T
                        {
                                error("Light vertex is a camera");
                        },
                        [&](const Light<N, T, Color>& light) -> T
                        {
                                return light.area_pdf(surface.pos(), surface.normal());
                        },
                        [&](const InfiniteLight<N, T, Color>& light) -> T
                        {
                                return light.area_pdf(surface.normal());
                        }},
                light_vertex);
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T compute_light_origin_pdf(
        const Vertex<N, T, Color>& light_vertex,
        const Vertex<N, T, Color>& /*next_vertex*/)
{
        return std::visit(
                Visitors{
                        [](const Surface<N, T, Color>&) -> T
                        {
                                error("Light vertex is a surface");
                        },
                        [](const Camera<N, T, Color>&) -> T
                        {
                                error("Light vertex is a camera");
                        },
                        [](const Light<N, T, Color>&) -> T
                        {
                                error("Light vertex is a light");
                        },
                        [&](const InfiniteLight<N, T, Color>& light) -> T
                        {
                                return light.pdf_reversed();
                        }},
                light_vertex);
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T compute_pdf(
        const Vertex<N, T, Color>& prev_vertex,
        const Vertex<N, T, Color>& vertex,
        const Vertex<N, T, Color>& next_vertex)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(vertex)));
        const auto& surface = std::get<Surface<N, T, Color>>(vertex);

        const Vector<N, T> to_prev = std::visit(
                Visitors{
                        [&](const Surface<N, T, Color>& prev) -> Vector<N, T>
                        {
                                return (prev.pos() - surface.pos()).normalized();
                        },
                        [&](const Camera<N, T, Color>& prev) -> Vector<N, T>
                        {
                                return (prev.pos() - surface.pos()).normalized();
                        },
                        [&](const Light<N, T, Color>& prev) -> Vector<N, T>
                        {
                                return prev.dir_to_light(surface.pos());
                        },
                        [](const InfiniteLight<N, T, Color>&) -> Vector<N, T>
                        {
                                error("Previous vertex is an infinite light");
                        }},
                prev_vertex);

        return std::visit(
                Visitors{
                        [&](const Surface<N, T, Color>& next) -> T
                        {
                                return next.reversed_pdf(to_prev, surface);
                        },
                        [](const Camera<N, T, Color>&) -> T
                        {
                                error("Next vertex is a camera");
                        },
                        [&](const Light<N, T, Color>& next) -> T
                        {
                                const Vector<N, T> to_next = next.dir_to_light(surface.pos());
                                return next.reversed_pdf(surface, surface.angle_pdf(to_prev, to_next));
                        },
                        [](const InfiniteLight<N, T, Color>&) -> T
                        {
                                error("Next vertex is an infinite light");
                        }},
                next_vertex);
}
}
