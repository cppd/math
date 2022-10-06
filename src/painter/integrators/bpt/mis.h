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

#include <vector>

namespace ns::painter::integrators::bpt
{
namespace mis_weight_implementation
{
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
                                        nodes->emplace_back(-1, -1, v.is_connectible());
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
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] T mis_weight(
        const std::vector<Vertex<N, T, Color>>& light_path,
        const std::vector<Vertex<N, T, Color>>& camera_path,
        const int s,
        const int t)
{
        namespace impl = mis_weight_implementation;

        ASSERT(t >= 2);
        ASSERT(s >= 0);

        if (s + t == 2)
        {
                return 1;
        }

        thread_local std::vector<impl::Node<T>> light_nodes;
        thread_local std::vector<impl::Node<T>> camera_nodes;

        impl::make_nodes(light_path, s, &light_nodes);
        impl::make_nodes(camera_path, t, &camera_nodes);

        return 1;
}
}
