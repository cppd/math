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

#include "connect.h"

#include "mis.h"

#include "../com/functions.h"
#include "../com/visibility.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
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
std::optional<Color> compute_color_s_1(
        const Surface<N, T, Color>& surface,
        const Ray<N, T>& ray_to_light,
        const LightDistributionSample<N, T, Color>& distribution,
        const LightSourceArriveSample<N, T, Color>& sample)
{
        const Vector<N, T>& n = surface.normal();
        const Vector<N, T>& v = surface.dir_to_prev();
        const Vector<N, T>& l = ray_to_light.dir();

        const T n_l = dot(n, l);
        if (!(n_l > 0))
        {
                return {};
        }

        return surface.beta() * surface.brdf(v, l) * sample.radiance * (n_l / (sample.pdf * distribution.pdf));
}

template <std::size_t N, typename T, typename Color>
std::optional<ConnectS1<N, T, Color>> connect_s_1(
        const Scene<N, T, Color>& scene,
        const Vertex<N, T, Color>& camera_vertex,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_vertex)));
        const auto& surface = std::get<Surface<N, T, Color>>(camera_vertex);
        if (!surface.is_connectible())
        {
                return {};
        }

        const LightDistributionSample distribution = light_distribution.sample(engine);

        const LightSourceArriveSample sample =
                distribution.light->arrive_sample(engine, surface.pos(), surface.normal());
        if (!sample.usable())
        {
                return {};
        }

        const Light<N, T, Color> light = [&]()
        {
                const auto position =
                        sample.distance ? std::optional<Vector<N, T>>(surface.pos() + sample.l * (*sample.distance))
                                        : std::nullopt;

                return Light<N, T, Color>(
                        distribution.light, distribution.pdf, sample.pdf, position, -sample.l, std::nullopt, surface);
        }();

        const Ray<N, T> ray_to_light(surface.pos(), sample.l);

        const auto color = compute_color_s_1(surface, ray_to_light, distribution, sample);
        if (!color || color->is_black())
        {
                return {};
        }

        if (occluded(scene, surface.normals(), ray_to_light, sample.distance))
        {
                return {};
        }

        return ConnectS1<N, T, Color>{.color = *color, .light_vertex = light};
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> compute_color(const Surface<N, T, Color>& light, const Surface<N, T, Color>& camera)
{
        const Vector<N, T> v = light.pos() - camera.pos();
        const T distance = v.norm();
        const Vector<N, T> from_camera_to_light = v / distance;

        const Vector<N, T>& camera_n = camera.normal();
        const Vector<N, T>& camera_v = camera.dir_to_prev();
        const Vector<N, T>& camera_l = from_camera_to_light;

        const T camera_n_l = dot(camera_n, camera_l);
        if (!(camera_n_l > 0))
        {
                return {};
        }

        const Vector<N, T>& light_n = light.normal();
        const Vector<N, T>& light_v = light.dir_to_prev();
        const Vector<N, T>& light_l = -from_camera_to_light;

        const T light_n_l = dot(light_n, light_l);
        if (!(light_n_l > 0))
        {
                return {};
        }

        const Color c = camera.beta() * camera.brdf(camera_v, camera_l) * light.brdf(light_v, light_l) * light.beta();

        const T g = camera_n_l * light_n_l / power<N - 1>(distance);

        return c * g;
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const Vertex<N, T, Color>& light_vertex,
        const Vertex<N, T, Color>& camera_vertex)
{
        ASSERT((std::holds_alternative<Surface<N, T, Color>>(light_vertex)));
        const auto& light = std::get<Surface<N, T, Color>>(light_vertex);
        if (!light.is_connectible())
        {
                return {};
        }

        ASSERT((std::holds_alternative<Surface<N, T, Color>>(camera_vertex)));
        const auto& camera = std::get<Surface<N, T, Color>>(camera_vertex);
        if (!camera.is_connectible())
        {
                return {};
        }

        const auto color = compute_color(light, camera);
        if (!color || color->is_black())
        {
                return {};
        }

        if (occluded(scene, camera.pos(), camera.normals(), light.pos(), light.normals()))
        {
                return {};
        }

        return *color;
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
