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

#include "mis.h"
#include "vertex.h"

#include "../../objects.h"
#include "../com/visibility.h"

#include <src/com/error.h>

#include <optional>
#include <vector>

namespace ns::painter::integrators::bpt
{
namespace connect_implementation
{
template <std::size_t N, typename T, typename Color>
struct ConnectS1 final
{
        Color color;
        Light<N, T, Color> light_vertex;
};

template <
        std::size_t N,
        typename T,
        typename Color,
        template <std::size_t, typename, typename>
        typename CameraPathVertex>
std::optional<ConnectS1<N, T, Color>> connect_s_1(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        const CameraPathVertex<N, T, Color>& camera_path_prev_vertex,
        const Surface<N, T, Color>& camera_path_vertex)
{
        if (!camera_path_vertex.is_connectible())
        {
                return {};
        }

        const LightDistributionSample distribution = light_distribution.sample(engine);

        const LightSourceSample<N, T, Color> sample = distribution.light->sample(engine, camera_path_vertex.pos());
        if (!sample.usable())
        {
                return {};
        }

        ASSERT(sample.distance);
        const Light<N, T, Color> light_vertex(
                distribution.light, camera_path_vertex.pos() + sample.l * (*sample.distance), std::nullopt,
                sample.radiance / (sample.pdf * distribution.pdf), light_distribution, camera_path_vertex);

        const auto [color, to_light] = [&]
        {
                const Vector<N, T>& p = camera_path_vertex.pos();
                const Vector<N, T>& n = camera_path_vertex.normal();
                const Ray<N, T> l(p, light_vertex.pos() - p);
                const Ray<N, T> v(p, camera_path_prev_vertex.pos() - p);
                const T n_l = std::abs(dot(n, l.dir()));
                const Color brdf = camera_path_vertex.brdf(v.dir(), l.dir());

                return std::make_tuple(camera_path_vertex.beta() * brdf * light_vertex.beta() * n_l, l);
        }();

        if (color.is_black())
        {
                return {};
        }

        if (light_source_occluded(scene, camera_path_vertex.normals(), to_light, sample.distance))
        {
                return {};
        }

        return ConnectS1<N, T, Color>{.color = color, .light_vertex = light_vertex};
}

template <
        std::size_t N,
        typename T,
        typename Color,
        template <std::size_t, typename, typename>
        typename LightPathVertex,
        template <std::size_t, typename, typename>
        typename CameraPathVertex>
std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const LightPathVertex<N, T, Color>& light_path_prev_vertex,
        const Surface<N, T, Color>& light_path_vertex,
        const CameraPathVertex<N, T, Color>& camera_path_prev_vertex,
        const Surface<N, T, Color>& camera_path_vertex)
{
        if (!light_path_vertex.is_connectible() || !camera_path_vertex.is_connectible())
        {
                return {};
        }

        const Color color = [&]
        {
                const Vector<N, T>& p_0 = camera_path_vertex.pos();
                const Vector<N, T>& n_0 = camera_path_vertex.normal();
                const Vector<N, T> l_0 = (light_path_vertex.pos() - p_0).normalized();
                const Vector<N, T> v_0 = (camera_path_prev_vertex.pos() - p_0).normalized();

                const Vector<N, T>& p_1 = light_path_vertex.pos();
                const Vector<N, T>& n_1 = light_path_vertex.normal();
                const Vector<N, T> l_1 = -l_0;
                const Vector<N, T> v_1 = (light_path_prev_vertex.pos() - p_1).normalized();

                const T n_l_0 = std::abs(dot(n_0, l_0));
                const T n_l_1 = std::abs(dot(n_1, l_1));

                const Color brdf = camera_path_vertex.brdf(v_0, l_0) * light_path_vertex.brdf(v_1, l_1);

                return camera_path_vertex.beta() * brdf * light_path_vertex.beta() * (n_l_0 * n_l_1);
        }();

        if (color.is_black())
        {
                return {};
        }

        if (occluded(
                    scene, camera_path_vertex.pos(), camera_path_vertex.normals(), light_path_vertex.pos(),
                    light_path_vertex.normals()))
        {
                return {};
        }

        return color;
}

//

template <std::size_t N, typename T, typename Color>
decltype(auto) connect_s_1(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        const Vertex<N, T, Color>& camera_path_prev_vertex,
        const Vertex<N, T, Color>& camera_path_vertex)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_path_vertex)));
        const auto& camera_vertex = std::get<Surface<N, T, Color>>(camera_path_vertex);

        return std::visit(
                [&](const auto& camera_prev_vertex)
                {
                        return connect_s_1(scene, light_distribution, engine, camera_prev_vertex, camera_vertex);
                },
                camera_path_prev_vertex);
}

template <std::size_t N, typename T, typename Color>
decltype(auto) connect(
        const Scene<N, T, Color>& scene,
        const Vertex<N, T, Color>& light_path_prev_vertex,
        const Vertex<N, T, Color>& light_path_vertex,
        const Vertex<N, T, Color>& camera_path_prev_vertex,
        const Vertex<N, T, Color>& camera_path_vertex)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(light_path_vertex)));
        const auto& light_vertex = std::get<Surface<N, T, Color>>(light_path_vertex);

        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_path_vertex)));
        const auto& camera_vertex = std::get<Surface<N, T, Color>>(camera_path_vertex);

        return std::visit(
                [&](const auto& light_prev_vertex)
                {
                        return std::visit(
                                [&](const auto& camera_prev_vertex)
                                {
                                        return connect(
                                                scene, light_prev_vertex, light_vertex, camera_prev_vertex,
                                                camera_vertex);
                                },
                                camera_path_prev_vertex);
                },
                light_path_prev_vertex);
}
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        const std::vector<Vertex<N, T, Color>>& light_path,
        const std::vector<Vertex<N, T, Color>>& camera_path,
        const int s,
        const int t)
{
        namespace impl = connect_implementation;

        ASSERT(s >= 1);
        ASSERT(t >= 2);

        std::optional<Color> color;
        std::optional<Light<N, T, Color>> vertex;

        if (s == 1)
        {
                const Vertex<N, T, Color>& t_2 = camera_path[t - 2];
                const Vertex<N, T, Color>& t_1 = camera_path[t - 1];

                auto connection = impl::connect_s_1(scene, light_distribution, engine, t_2, t_1);
                if (connection)
                {
                        color = std::move(connection->color);
                        vertex = std::move(connection->light_vertex);
                }
        }
        else
        {
                const Vertex<N, T, Color>& s_2 = light_path[t - 2];
                const Vertex<N, T, Color>& s_1 = light_path[t - 1];
                const Vertex<N, T, Color>& t_2 = camera_path[t - 2];
                const Vertex<N, T, Color>& t_1 = camera_path[t - 1];

                color = impl::connect(scene, s_2, s_1, t_2, t_1);
        }

        if (!color || color->is_black())
        {
                return {};
        }

        *color *= mis_weight(light_path, camera_path, s, t, vertex);

        return color;
}
}
