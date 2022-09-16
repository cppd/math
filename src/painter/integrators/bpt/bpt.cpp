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

#include "bpt.h"

#include "connect.h"
#include "surface_sample.h"
#include "vertex.h"

#include "../com/normals.h"
#include "../com/visibility.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <vector>

namespace ns::painter::integrators::bpt
{
namespace
{
constexpr int MAX_DEPTH = 5;

template <std::size_t N, typename T, typename Color>
[[nodiscard]] bool surface_found(
        const Ray<N, T>& ray,
        const SurfaceIntersection<N, T, Color>& surface,
        const Normals<N, T>& normals)
{
        return surface && dot(normals.shading, -ray.dir()) > 0;
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void walk(
        const Scene<N, T, Color>& scene,
        Color beta,
        const T pdf,
        Ray<N, T> ray,
        PCG& engine,
        std::vector<Vertex<N, T, Color>>* const path)
{
        ASSERT(!path->empty());

        SurfaceIntersection<N, T, Color> surface;
        Normals<N, T> normals;

        std::tie(surface, normals) = [&]
        {
                static constexpr std::optional<Vector<N, T>> GEOMETRIC_NORMAL;
                return scene_intersect<FLAT_SHADING, N, T, Color>(scene, GEOMETRIC_NORMAL, ray);
        }();

        if (!surface_found(ray, surface, normals))
        {
                return;
        }

        T pdf_forward = pdf;

        for (int depth = 0; depth < MAX_DEPTH; ++depth)
        {
                Vertex<N, T, Color>& next =
                        path->emplace_back(std::in_place_type<Surface<N, T, Color>>, surface, normals, beta);

                Vertex<N, T, Color>& prev = *(path->end() - 2);

                const auto sample = surface_sample(surface, -ray.dir(), normals, engine);
                if (!sample)
                {
                        set_forward_pdf(prev, &next, pdf_forward);
                        break;
                }

                if (!surface.is_specular())
                {
                        set_forward_pdf(prev, &next, pdf_forward);
                }
                set_reversed_pdf(&prev, next, sample->pdf_reversed);

                pdf_forward = sample->pdf_forward;
                beta *= sample->beta;

                if (beta.is_black())
                {
                        break;
                }

                ray = Ray<N, T>(surface.point(), sample->l);
                std::tie(surface, normals) = scene_intersect<FLAT_SHADING, N, T, Color>(scene, normals.geometric, ray);

                if (!surface_found(ray, surface, normals))
                {
                        break;
                }
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void generate_camera_path(
        const Scene<N, T, Color>& scene,
        const Ray<N, T>& ray,
        PCG& engine,
        std::vector<Vertex<N, T, Color>>* const path)
{
        path->clear();

        const Color beta(1);

        path->emplace_back(std::in_place_type<Camera<N, T, Color>>, ray.org(), beta);

        walk<FLAT_SHADING>(scene, beta, /*pdf=*/T{1}, ray, engine, path);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void generate_light_path(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        std::vector<Vertex<N, T, Color>>* const path)
{
        path->clear();

        const LightDistributionSample distribution = light_distribution.sample(engine);
        const LightSourceLeaveSample sample = distribution.light->leave_sample(engine);

        if (!(sample.pdf_pos > 0 && sample.pdf_dir > 0 && !sample.radiance.is_black()))
        {
                return;
        }

        path->emplace_back(
                std::in_place_type<Light<N, T, Color>>, distribution.light, sample.ray.org(), sample.n, sample.radiance,
                distribution.pdf * sample.pdf_pos);

        const T pdf = distribution.pdf * sample.pdf_pos * sample.pdf_dir;
        const T k = sample.n ? std::abs(dot(*sample.n, sample.ray.dir())) : 1;
        const Color beta = sample.radiance * (k / pdf);

        walk<FLAT_SHADING>(scene, beta, sample.pdf_dir, sample.ray, engine, path);
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> bpt(
        const Scene<N, T, Color>& scene,
        const Ray<N, T>& ray,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        thread_local std::vector<Vertex<N, T, Color>> camera_path;
        thread_local std::vector<Vertex<N, T, Color>> light_path;

        generate_camera_path<FLAT_SHADING>(scene, ray, engine, &camera_path);
        ASSERT(camera_path.size() <= MAX_DEPTH + 1);
        ASSERT(camera_path.size() >= 1);
        if (camera_path.size() == 1)
        {
                return {};
        }

        generate_light_path<FLAT_SHADING>(scene, light_distribution, engine, &light_path);
        ASSERT(light_path.size() <= MAX_DEPTH + 1);

        const int camera_size = camera_path.size();
        const int light_size = light_path.size();

        Color color(0);

        for (int t = 2; t <= camera_size; ++t)
        {
                for (int s = 1; s <= light_size; ++s)
                {
                        const int depth = t + s - 2;
                        if (depth > MAX_DEPTH)
                        {
                                continue;
                        }

                        const auto c = connect(scene, light_path, camera_path, s, t, light_distribution, engine);
                        if (c)
                        {
                                color += *c;
                        }
                }
        }

        return color;
}

#define TEMPLATE(N, T, C)                                                                        \
        template std::optional<C> bpt<true, (N), T, C>(                                          \
                const Scene<(N), T, C>&, const Ray<(N), T>&, LightDistribution<N, T, C>&, PCG&); \
        template std::optional<C> bpt<false, (N), T, C>(                                         \
                const Scene<(N), T, C>&, const Ray<(N), T>&, LightDistribution<N, T, C>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
