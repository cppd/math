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

#include "../../objects.h"
#include "../visibility.h"

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
        bool FLAT_SHADING,
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

        const Vector<N, T>& p = camera_path_vertex.pos();
        const Vector<N, T>& n = camera_path_vertex.normal();
        const Ray<N, T> l(p, light_vertex.pos() - p);
        const Ray<N, T> v(p, camera_path_prev_vertex.pos() - p);
        const T n_l = std::abs(dot(n, l.dir()));
        const Color color =
                camera_path_vertex.beta() * camera_path_vertex.brdf(v.dir(), l.dir()) * light_vertex.beta() * n_l;

        if (color.is_black())
        {
                return {};
        }

        if (light_source_occluded(scene, camera_path_vertex.normals(), l, sample.distance))
        {
                return {};
        }

        return ConnectS1<N, T, Color>{.color = color, .light_vertex = light_vertex};
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> connect(
        const Scene<N, T, Color>& /*scene*/,
        PCG& /*engine*/,
        const Surface<N, T, Color>& /*light_path_vertex*/,
        const Surface<N, T, Color>& /*camera_path_vertex*/,
        const int /*s*/,
        const int /*t*/)
{
        return {};
}

//

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
decltype(auto) connect_s_1(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        const Vertex<N, T, Color>& camera_path_prev_vertex,
        const Vertex<N, T, Color>& camera_path_vertex)
{
        return std::visit(
                [&](const auto& prev_vertex)
                {
                        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_path_vertex)));

                        return connect_s_1<FLAT_SHADING>(
                                scene, light_distribution, engine, prev_vertex,
                                std::get<Surface<N, T, Color>>(camera_path_vertex));
                },
                camera_path_prev_vertex);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
decltype(auto) connect(
        const Scene<N, T, Color>& scene,
        PCG& engine,
        const Vertex<N, T, Color>& light_path_vertex,
        const Vertex<N, T, Color>& camera_path_vertex,
        const int s,
        const int t)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(light_path_vertex)));
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_path_vertex)));

        return connect<FLAT_SHADING>(
                scene, engine, std::get<Surface<N, T, Color>>(light_path_vertex),
                std::get<Surface<N, T, Color>>(camera_path_vertex), s, t);
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
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
        std::optional<Vertex<N, T, Color>> vertex;

        if (s == 1)
        {
                auto connection = impl::connect_s_1<FLAT_SHADING>(
                        scene, light_distribution, engine, camera_path[t - 2], camera_path[t - 1]);
                if (connection)
                {
                        color = std::move(connection->color);
                        vertex = std::move(connection->light_vertex);
                }
        }
        else
        {
                color = impl::connect<FLAT_SHADING>(scene, engine, light_path[s - 1], camera_path[t - 1], s, t);
        }

        return color;
}
}
