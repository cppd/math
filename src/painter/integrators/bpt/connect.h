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

#include <src/com/error.h>

#include <optional>
#include <vector>

namespace ns::painter::integrators::bpt
{
namespace connect_implementation
{
template <typename T, typename Color>
bool use_pdf_color(const T pdf, const Color& color)
{
        return pdf > 0 && !color.is_black();
}

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
        const Scene<N, T, Color>& /*scene*/,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        const CameraPathVertex<N, T, Color>& camera_path_vertex)
{
        if (!camera_path_vertex.is_connectible())
        {
                return {};
        }

        const LightDistributionSample distribution = light_distribution.sample(engine);

        const LightSourceSample<N, T, Color> sample = distribution.light->sample(engine, camera_path_vertex.pos());
        if (!use_pdf_color(sample.pdf, sample.radiance))
        {
                return {};
        }

        ASSERT(sample.distance);
        Light<N, T, Color> light_vertex(
                distribution.light, camera_path_vertex.pos() + sample.l * (*sample.distance), std::nullopt,
                sample.radiance / (sample.pdf * distribution.pdf), light_distribution, camera_path_vertex);

        return ConnectS1<N, T, Color>{.color = Color(0), .light_vertex = std::move(light_vertex)};
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
decltype(auto) connect_s_1(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        const Vertex<N, T, Color>& camera_path_vertex)
{
        return std::visit(
                [&](const auto& cpv)
                {
                        return connect_s_1<FLAT_SHADING>(scene, light_distribution, engine, cpv);
                },
                camera_path_vertex);
}

//

template <
        bool FLAT_SHADING,
        std::size_t N,
        typename T,
        typename Color,
        template <std::size_t, typename, typename>
        typename LightPathVertex,
        template <std::size_t, typename, typename>
        typename CameraPathVertex>
std::optional<Color> connect(
        const Scene<N, T, Color>& /*scene*/,
        PCG& /*engine*/,
        const LightPathVertex<N, T, Color>& /*light_path_vertex*/,
        const CameraPathVertex<N, T, Color>& /*camera_path_vertex*/,
        const int /*s*/,
        const int /*t*/)
{
        return {};
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
        return std::visit(
                [&](const auto& lpv)
                {
                        return std::visit(
                                [&](const auto& cpv)
                                {
                                        return connect<FLAT_SHADING>(scene, engine, lpv, cpv, s, t);
                                },
                                camera_path_vertex);
                },
                light_path_vertex);
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

        ASSERT(t >= 2);
        ASSERT(s >= 1);

        std::optional<Color> color;
        std::optional<Vertex<N, T, Color>> vertex;

        if (s == 1)
        {
                auto connection =
                        impl::connect_s_1<FLAT_SHADING>(scene, light_distribution, engine, camera_path[t - 1]);
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
