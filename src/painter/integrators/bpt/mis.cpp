/*
Copyright (C) 2017-2023 Topological Manifold

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

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

13.10.1 Multiple importance sampling
16.3.4 Multiple importance sampling
*/

#include "mis.h"

#include "vertex_pdf.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/variant.h>
#include <src/settings/instantiation.h>

#include <vector>

namespace ns::painter::integrators::bpt
{
namespace
{
template <typename T>
constexpr T EMPTY = -1;

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
void make_nodes(
        const std::vector<vertex::Vertex<N, T, Color>>& path,
        const int count,
        std::vector<Node<T>>* const nodes)
{
        nodes->clear();

        for (int i = 0; i < count; ++i)
        {
                std::visit(
                        Visitors{
                                [&](const vertex::Surface<N, T, Color>& v)
                                {
                                        nodes->emplace_back(v.pdf_forward(), v.pdf_reversed(), v.is_connectible());
                                },
                                [&](const vertex::Camera<N, T, Color>& v)
                                {
                                        nodes->emplace_back(EMPTY<T>, EMPTY<T>, v.is_connectible());
                                },
                                [&](const vertex::Light<N, T, Color>& v)
                                {
                                        nodes->emplace_back(v.pdf_forward(), v.pdf_reversed(), v.is_connectible());
                                },
                                [&](const vertex::InfiniteLight<N, T, Color>& v)
                                {
                                        nodes->emplace_back(v.pdf_forward(), v.pdf_origin(), v.is_connectible());
                                }},
                        path[i]);
        }
}

template <typename T>
void set_connectible(std::vector<Node<T>>* const nodes)
{
        if (!nodes->empty())
        {
                nodes->back().connectible = true;
        }
}

template <std::size_t N, typename T, typename Color>
void set_reversed(
        const std::vector<vertex::Vertex<N, T, Color>>& light,
        const std::vector<vertex::Vertex<N, T, Color>>& camera,
        const int s,
        const int t,
        std::vector<Node<T>>* const light_nodes,
        std::vector<Node<T>>* const camera_nodes)
{
        ASSERT(s >= 0);
        ASSERT(t >= 2);

        if (s == 0)
        {
                ASSERT(t > 2);
                (*camera_nodes)[t - 1].reversed = compute_light_origin_pdf(camera[t - 1]);
                (*camera_nodes)[t - 2].reversed = compute_light_pdf(camera[t - 1], camera[t - 2]);
                return;
        }

        (*light_nodes)[s - 1].reversed = compute_pdf(camera[t - 2], camera[t - 1], light[s - 1]);

        if (t > 2)
        {
                (*camera_nodes)[t - 2].reversed = compute_pdf(light[s - 1], camera[t - 1], camera[t - 2]);
        }

        if (s == 1)
        {
                (*camera_nodes)[t - 1].reversed = compute_light_pdf(light[s - 1], camera[t - 1]);
        }
        else if (s > 1)
        {
                (*light_nodes)[s - 2].reversed = compute_pdf(camera[t - 1], light[s - 1], light[s - 2]);
                (*camera_nodes)[t - 1].reversed = compute_pdf(light[s - 2], light[s - 1], camera[t - 1]);
        }

        ASSERT((*camera_nodes)[0].forward == EMPTY<T>);
        ASSERT((*camera_nodes)[0].reversed == EMPTY<T>);
}

template <typename T>
[[nodiscard]] T map(const T v)
{
        ASSERT(v >= 0);
        return v != 0 ? v : 1;
}

template <typename T>
[[nodiscard]] T light_sum(const std::vector<Node<T>>& light)
{
        if (light.empty())
        {
                return 0;
        }

        T sum = 0;
        T ri = 1;
        for (int i = light.size() - 1; i > 0; --i)
        {
                ri *= map(light[i].reversed) / map(light[i].forward);
                if (light[i].connectible && light[i - 1].connectible)
                {
                        sum += ri;
                }
        }
        if (light[0].connectible)
        {
                ri *= map(light[0].reversed) / map(light[0].forward);
                sum += ri;
        }
        return sum;
}

template <typename T>
[[nodiscard]] T camera_sum(const std::vector<Node<T>>& camera)
{
        if (camera.size() <= 1)
        {
                return 0;
        }

        T sum = 0;
        T ri = 1;
        for (int i = camera.size() - 1; i > 1; --i)
        {
                ri *= map(camera[i].reversed) / map(camera[i].forward);
                if (camera[i].connectible && camera[i - 1].connectible)
                {
                        sum += ri;
                }
        }
        if (camera[1].connectible && camera[0].connectible)
        {
                ri *= map(camera[1].reversed) / map(camera[1].forward);
                sum += ri;
        }
        return sum;
}
}

template <std::size_t N, typename T, typename Color>
T mis_weight(
        const std::vector<vertex::Vertex<N, T, Color>>& light_path,
        const std::vector<vertex::Vertex<N, T, Color>>& camera_path,
        const int s,
        const int t)
{
        ASSERT(s >= 0);
        ASSERT(t >= 2);

        if (s + t == 2)
        {
                return 1;
        }

        thread_local std::vector<Node<T>> light_nodes;
        thread_local std::vector<Node<T>> camera_nodes;

        make_nodes(light_path, s, &light_nodes);
        make_nodes(camera_path, t, &camera_nodes);

        set_reversed(light_path, camera_path, s, t, &light_nodes, &camera_nodes);

        set_connectible(&light_nodes);
        set_connectible(&camera_nodes);

        return 1 / (1 + light_sum(light_nodes) + camera_sum(camera_nodes));
}

#define TEMPLATE(N, T, C)                                                                                          \
        template T mis_weight(                                                                                     \
                const std::vector<vertex::Vertex<(N), T, C>>&, const std::vector<vertex::Vertex<(N), T, C>>&, int, \
                int);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
