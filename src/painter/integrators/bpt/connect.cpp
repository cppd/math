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

#include "connect.h"

#include "mis.h"

#include "../com/functions.h"
#include "../com/visibility.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/variant.h>
#include <src/settings/instantiation.h>

namespace ns::painter::integrators::bpt
{
namespace
{
template <std::size_t N, typename T, typename Color>
std::optional<Color> connect_s_0(const Scene<N, T, Color>& scene, const Vertex<N, T, Color>& camera_path_vertex)
{
        return std::visit(
                Visitors{
                        [](const Surface<N, T, Color>& surface) -> std::optional<Color>
                        {
                                if (!surface.is_light())
                                {
                                        return {};
                                }
                                if (const auto& radiance = surface.light_radiance())
                                {
                                        return *radiance * surface.beta();
                                }
                                return {};
                        },
                        [](const Camera<N, T, Color>&) -> std::optional<Color>
                        {
                                error("Last camera path vertex is a camera");
                        },
                        [](const Light<N, T, Color>&) -> std::optional<Color>
                        {
                                error("Last camera path vertex is an light");
                        },
                        [&scene](const InfiniteLight<N, T, Color>& infinite_light) -> std::optional<Color>
                        {
                                std::optional<Color> res;
                                for (const LightSource<N, T, Color>* const light : scene.light_sources())
                                {
                                        if (!light->is_infinite_area())
                                        {
                                                continue;
                                        }
                                        if (const auto& radiance = light->leave_radiance(infinite_light.dir()))
                                        {
                                                com::add_optional(&res, *radiance * infinite_light.beta());
                                        }
                                }
                                return res;
                        }},
                camera_path_vertex);
}

template <std::size_t N, typename T, typename Color>
struct ConnectS1 final
{
        Color color;
        Light<N, T, Color> light_vertex;
};

template <std::size_t N, typename T, typename Color>
std::optional<ConnectS1<N, T, Color>> connect_s_1(
        const Scene<N, T, Color>& scene,
        const Vertex<N, T, Color>& camera_path_vertex,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_path_vertex)));
        const auto& camera_vertex = std::get<Surface<N, T, Color>>(camera_path_vertex);

        if (!camera_vertex.is_connectible())
        {
                return {};
        }

        const LightDistributionSample distribution = light_distribution.sample(engine);

        const LightSourceArriveSample<N, T, Color> sample =
                distribution.light->arrive_sample(engine, camera_vertex.pos(), camera_vertex.normal());
        if (!sample.usable())
        {
                return {};
        }

        const Light<N, T, Color> light_vertex(
                distribution.light, distribution.pdf, sample.pdf,
                sample.distance ? (camera_vertex.pos() + sample.l * (*sample.distance)) : std::optional<Vector<N, T>>(),
                -sample.l, std::nullopt, sample.radiance / (sample.pdf * distribution.pdf), camera_vertex);

        const Ray<N, T> ray_to_light(camera_vertex.pos(), sample.l);

        const Color color = [&]
        {
                const Vector<N, T>& n = camera_vertex.normal();
                const Vector<N, T>& v = camera_vertex.dir_to_prev();
                const Vector<N, T>& l = ray_to_light.dir();

                const Color& brdf = camera_vertex.brdf(v, l);

                const T n_l = std::abs(dot(n, l));

                return camera_vertex.beta() * brdf * light_vertex.beta() * n_l;
        }();

        if (color.is_black())
        {
                return {};
        }

        if (light_source_occluded(scene, camera_vertex.normals(), ray_to_light, sample.distance))
        {
                return {};
        }

        return ConnectS1<N, T, Color>{.color = color, .light_vertex = light_vertex};
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const Vertex<N, T, Color>& light_path_vertex,
        const Vertex<N, T, Color>& camera_path_vertex)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(light_path_vertex)));
        const auto& light_vertex = std::get<Surface<N, T, Color>>(light_path_vertex);

        if (!light_vertex.is_connectible())
        {
                return {};
        }

        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_path_vertex)));
        const auto& camera_vertex = std::get<Surface<N, T, Color>>(camera_path_vertex);

        if (!camera_vertex.is_connectible())
        {
                return {};
        }

        const Color color = [&]
        {
                const Vector<N, T>& from_camera_to_light = (light_vertex.pos() - camera_vertex.pos()).normalized();

                const Vector<N, T>& camera_n = camera_vertex.normal();
                const Vector<N, T>& camera_v = camera_vertex.dir_to_prev();
                const Vector<N, T>& camera_l = from_camera_to_light;

                const Vector<N, T>& light_n = light_vertex.normal();
                const Vector<N, T>& light_v = light_vertex.dir_to_prev();
                const Vector<N, T>& light_l = -from_camera_to_light;

                const Color& brdf = camera_vertex.brdf(camera_v, camera_l) * light_vertex.brdf(light_v, light_l);

                const T camera_n_l = std::abs(dot(camera_n, camera_l));
                const T light_n_l = std::abs(dot(light_n, light_l));

                return camera_vertex.beta() * brdf * light_vertex.beta() * (camera_n_l * light_n_l);
        }();

        if (color.is_black())
        {
                return {};
        }

        if (occluded(scene, camera_vertex.pos(), camera_vertex.normals(), light_vertex.pos(), light_vertex.normals()))
        {
                return {};
        }

        return color;
}
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const std::vector<Vertex<N, T, Color>>& light_path,
        const std::vector<Vertex<N, T, Color>>& camera_path,
        const int s,
        const int t,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        ASSERT(s >= 0);
        ASSERT(t >= 2);

        std::optional<Color> color;
        const std::vector<Vertex<N, T, Color>>* connected_light_path = &light_path;

        if (s == 0)
        {
                color = connect_s_0(scene, camera_path[t - 1]);
        }
        else if (std::holds_alternative<InfiniteLight<N, T, Color>>(camera_path[t - 1]))
        {
                return {};
        }
        else if (s == 1)
        {
                auto connection = connect_s_1(scene, camera_path[t - 1], light_distribution, engine);
                if (!connection)
                {
                        return {};
                }
                color = std::move(connection->color);
                thread_local std::vector<Vertex<N, T, Color>> path;
                path.clear();
                path.push_back(std::move(connection->light_vertex));
                connected_light_path = &path;
        }
        else
        {
                color = connect(scene, light_path[s - 1], camera_path[t - 1]);
        }

        if (!color || color->is_black())
        {
                return {};
        }

        *color *= mis_weight(*connected_light_path, camera_path, s, t);

        return color;
}

#define TEMPLATE(N, T, C)                                                                                              \
        template std::optional<C> connect(                                                                             \
                const Scene<(N), T, C>&, const std::vector<Vertex<(N), T, C>>&, const std::vector<Vertex<(N), T, C>>&, \
                int, int, LightDistribution<(N), T, C>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
