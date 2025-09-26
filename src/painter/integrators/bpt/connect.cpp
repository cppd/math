/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "light_distribution.h"
#include "mis.h"

#include "vertex/camera.h"
#include "vertex/infinite_light.h"
#include "vertex/light.h"
#include "vertex/surface.h"
#include "vertex/vertex.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>
#include <src/com/variant.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/integrators/com/functions.h>
#include <src/painter/integrators/com/visibility.h>
#include <src/painter/objects.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::integrators::bpt
{
namespace
{
template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<Color> connect_s_0(
        const Scene<N, T, Color>& scene,
        const vertex::Vertex<N, T, Color>& camera_path_vertex)
{
        return std::visit(
                Visitors{
                        [](const vertex::Surface<N, T, Color>& surface) -> std::optional<Color>
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
                        [](const vertex::Camera<N, T, Color>&) -> std::optional<Color>
                        {
                                error("Last camera path vertex is a camera");
                        },
                        [](const vertex::Light<N, T, Color>&) -> std::optional<Color>
                        {
                                error("Last camera path vertex is an light");
                        },
                        [&scene](const vertex::InfiniteLight<N, T, Color>& infinite_light) -> std::optional<Color>
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
        vertex::Light<N, T, Color> light_vertex;
};

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<Color> compute_color_s_1(
        const vertex::Surface<N, T, Color>& surface,
        const numerical::Ray<N, T>& ray_to_light,
        const LightDistributionSample<N, T, Color>& distribution,
        const LightSourceArriveSample<N, T, Color>& sample)
{
        const numerical::Vector<N, T>& n = surface.normal();
        const numerical::Vector<N, T>& v = surface.dir_to_prev();
        const numerical::Vector<N, T>& l = ray_to_light.dir();

        const T n_l = dot(n, l);
        if (!(n_l > 0))
        {
                return {};
        }

        return surface.beta() * surface.brdf(v, l) * sample.radiance * (n_l / (sample.pdf * distribution.pdf));
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<ConnectS1<N, T, Color>> connect_s_1(
        const Scene<N, T, Color>& scene,
        const vertex::Vertex<N, T, Color>& camera_vertex,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        ASSERT((std::holds_alternative<vertex::Surface<N, T, Color>>(camera_vertex)));
        const auto& surface = std::get<vertex::Surface<N, T, Color>>(camera_vertex);
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

        const vertex::Light<N, T, Color> light = [&]
        {
                const auto position =
                        sample.distance
                                ? std::optional<numerical::Vector<N, T>>(surface.pos() + sample.l * (*sample.distance))
                                : std::nullopt;

                return vertex::Light<N, T, Color>(
                        distribution.light, distribution.pdf, sample.pdf, position, -sample.l, std::nullopt, surface);
        }();

        const numerical::Ray<N, T> ray_to_light(surface.pos(), sample.l);

        const auto color = compute_color_s_1(surface, ray_to_light, distribution, sample);
        if (!color || color->is_black())
        {
                return {};
        }

        if (occluded(scene, surface.normals(), ray_to_light, sample.distance))
        {
                return {};
        }

        return {
                {.color = *color, .light_vertex = light}
        };
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::optional<Color> compute_color(
        const vertex::Surface<N, T, Color>& light,
        const vertex::Surface<N, T, Color>& camera)
{
        const numerical::Vector<N, T> v = light.pos() - camera.pos();
        const T distance = v.norm();
        const numerical::Vector<N, T> from_camera_to_light = v / distance;

        const numerical::Vector<N, T>& camera_n = camera.normal();
        const numerical::Vector<N, T>& camera_v = camera.dir_to_prev();
        const numerical::Vector<N, T>& camera_l = from_camera_to_light;

        const T camera_n_l = dot(camera_n, camera_l);
        if (!(camera_n_l > 0))
        {
                return {};
        }

        const numerical::Vector<N, T>& light_n = light.normal();
        const numerical::Vector<N, T>& light_v = light.dir_to_prev();
        const numerical::Vector<N, T>& light_l = -from_camera_to_light;

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
[[nodiscard]] std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const vertex::Vertex<N, T, Color>& light_vertex,
        const vertex::Vertex<N, T, Color>& camera_vertex)
{
        ASSERT((std::holds_alternative<vertex::Surface<N, T, Color>>(light_vertex)));
        const auto& light = std::get<vertex::Surface<N, T, Color>>(light_vertex);
        if (!light.is_connectible())
        {
                return {};
        }

        ASSERT((std::holds_alternative<vertex::Surface<N, T, Color>>(camera_vertex)));
        const auto& camera = std::get<vertex::Surface<N, T, Color>>(camera_vertex);
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

        return color;
}
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> connect(
        const Scene<N, T, Color>& scene,
        const std::vector<vertex::Vertex<N, T, Color>>& light_path,
        const std::vector<vertex::Vertex<N, T, Color>>& camera_path,
        const int s,
        const int t,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        ASSERT(s >= 0);
        ASSERT(t >= 2);

        const auto make_result = [&](const std::optional<Color>& color) -> std::optional<Color>
        {
                if (!color || color->is_black())
                {
                        return {};
                }
                return *color * mis_weight(light_path, camera_path, s, t);
        };

        if (s == 0)
        {
                return make_result(connect_s_0(scene, camera_path[t - 1]));
        }

        if (std::holds_alternative<vertex::InfiniteLight<N, T, Color>>(camera_path[t - 1]))
        {
                return {};
        }

        if (s == 1)
        {
                auto connection = connect_s_1(scene, camera_path[t - 1], light_distribution, engine);
                if (!connection)
                {
                        return {};
                }
                if (connection->color.is_black())
                {
                        return {};
                }
                thread_local std::vector<vertex::Vertex<N, T, Color>> path;
                path.clear();
                path.push_back(std::move(connection->light_vertex));
                return connection->color * mis_weight(path, camera_path, s, t);
        }

        return make_result(connect(scene, light_path[s - 1], camera_path[t - 1]));
}

#define TEMPLATE(N, T, C)                                                               \
        template std::optional<C> connect(                                              \
                const Scene<(N), T, C>&, const std::vector<vertex::Vertex<(N), T, C>>&, \
                const std::vector<vertex::Vertex<(N), T, C>>&, int, int, LightDistribution<(N), T, C>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
