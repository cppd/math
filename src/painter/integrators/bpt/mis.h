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
#include "vertex_pdf.h"

#include <src/com/error.h>
#include <src/com/variant.h>

#include <vector>

namespace ns::painter::integrators::bpt
{
namespace mis_weight_implementation
{
template <typename T>
inline constexpr T EMPTY = -1;

template <typename T>
struct Node
{
        T forward;
        T reversed;
        bool connectible;

        Node(const T forward, const T reversed, const bool connectible)
                : forward(forward),
                  reversed(reversed),
                  connectible(connectible)
        {
        }
};

template <std::size_t N, typename T, typename Color>
void make_nodes(const std::vector<Vertex<N, T, Color>>& path, const int count, std::vector<Node<T>>* const nodes)
{
        nodes->clear();

        for (int i = 0; i < count; ++i)
        {
                std::visit(
                        Visitors{
                                [&](const Surface<N, T, Color>& v)
                                {
                                        nodes->emplace_back(v.pdf_forward(), v.pdf_reversed(), v.is_connectible());
                                },
                                [&](const Camera<N, T, Color>& v)
                                {
                                        nodes->emplace_back(EMPTY<T>, EMPTY<T>, v.is_connectible());
                                },
                                [&](const Light<N, T, Color>& v)
                                {
                                        nodes->emplace_back(v.pdf_forward(), v.pdf_reversed(), v.is_connectible());
                                },
                                [&](const InfiniteLight<N, T, Color>& v)
                                {
                                        nodes->emplace_back(v.pdf_forward(), v.pdf_reversed(), v.is_connectible());
                                }},
                        path[i]);
        }
}

template <std::size_t N, typename T, typename Color>
void set_reversed(
        const std::vector<Vertex<N, T, Color>>& light,
        const std::vector<Vertex<N, T, Color>>& camera,
        const int s,
        const int t,
        std::vector<Node<T>>& light_nodes,
        std::vector<Node<T>>& camera_nodes)
{
        ASSERT(s >= 0);
        ASSERT(t >= 2);

        if (s == 0)
        {
                return;
        }

        light_nodes[s - 1].reversed = compute_pdf(camera[t - 2], camera[t - 1], light[s - 1]);

        if (t > 2)
        {
                camera_nodes[t - 2].reversed = compute_pdf(light[s - 1], camera[t - 1], camera[t - 2]);
        }

        if (s == 1)
        {
                camera_nodes[t - 1].reversed = compute_pdf(light[s - 1], camera[t - 1]);
        }
        else if (s > 1)
        {
                light_nodes[s - 2].reversed = compute_pdf(camera[t - 1], light[s - 1], light[s - 2]);
                camera_nodes[t - 1].reversed = compute_pdf(light[s - 2], light[s - 1], camera[t - 1]);
        }

        ASSERT(camera_nodes[0].forward == EMPTY<T>);
        ASSERT(camera_nodes[0].reversed == EMPTY<T>);
}
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T mis_weight(
        const std::vector<Vertex<N, T, Color>>& light_path,
        const std::vector<Vertex<N, T, Color>>& camera_path,
        const int s,
        const int t)
{
        namespace impl = mis_weight_implementation;

        ASSERT(s >= 0);
        ASSERT(t >= 2);

        if (s + t == 2)
        {
                return 1;
        }

        thread_local std::vector<impl::Node<T>> light_nodes;
        thread_local std::vector<impl::Node<T>> camera_nodes;

        impl::make_nodes(light_path, s, &light_nodes);
        impl::make_nodes(camera_path, t, &camera_nodes);

        impl::set_reversed(light_path, camera_path, s, t, light_nodes, camera_nodes);

        return 1;
}
}
