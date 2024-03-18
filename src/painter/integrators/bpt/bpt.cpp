/*
Copyright (C) 2017-2024 Topological Manifold

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

16.1 The path-space measurement equation
16.3 Bidirectional path tracing
*/

#include "bpt.h"

#include "connect.h"
#include "light_distribution.h"
#include "vertex_pdf.h"

#include "vertex/camera.h"
#include "vertex/infinite_light.h"
#include "vertex/light.h"
#include "vertex/surface.h"
#include "vertex/vertex.h"

#include <src/com/error.h>
#include <src/com/random/pcg.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/integrators/com/normals.h>
#include <src/painter/integrators/com/surface_sample.h>
#include <src/painter/integrators/com/visibility.h>
#include <src/painter/objects.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::integrators::bpt
{
namespace
{
constexpr int MAX_DEPTH = 5;

template <std::size_t N, typename T, typename Color>
[[nodiscard]] bool surface_found(
        const numerical::Ray<N, T>& ray,
        const SurfaceIntersection<N, T, Color>& surface,
        const Normals<N, T>& normals)
{
        return surface && dot(normals.shading, -ray.dir()) > 0;
}

template <std::size_t N, typename T>
[[nodiscard]] T correct_normals(
        const Normals<N, T>& normals,
        const numerical::Vector<N, T>& v,
        const numerical::Vector<N, T>& l)
{
        const T denominator = dot(v, normals.geometric) * dot(l, normals.shading);
        if (denominator == 0)
        {
                return 0;
        }
        const T numerator = dot(v, normals.shading) * dot(l, normals.geometric);
        return std::abs(numerator / denominator);
}

template <std::size_t N, typename T, typename Color>
void add_sample(
        const std::optional<SurfaceSamplePdf<N, T, Color>>& sample,
        const Color& beta,
        const T pdf_forward,
        const numerical::Ray<N, T>& ray,
        const SurfaceIntersection<N, T, Color>& surface,
        const Normals<N, T>& normals,
        std::vector<vertex::Vertex<N, T, Color>>* const path)
{
        if (!sample)
        {
                if (surface.light_source())
                {
                        vertex::Surface<N, T, Color> next(surface, normals, beta, -ray.dir());
                        set_forward_pdf(path->back(), &next, pdf_forward);
                        path->push_back(std::move(next));
                }
                return;
        }

        vertex::Surface<N, T, Color> next(surface, normals, beta, -ray.dir());
        vertex::Vertex<N, T, Color>& prev = path->back();
        set_forward_pdf(prev, &next, pdf_forward);
        set_reversed_pdf(&prev, std::as_const(next), sample->pdf_reversed);
        path->push_back(std::move(next));
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void walk(
        const bool camera_path,
        const Scene<N, T, Color>& scene,
        const LightDistribution<N, T, Color>& light_distribution,
        Color beta,
        const T pdf,
        numerical::Ray<N, T> ray,
        PCG& engine,
        std::vector<vertex::Vertex<N, T, Color>>* const path)
{
        ASSERT(!path->empty());

        SurfaceIntersection<N, T, Color> surface;
        Normals<N, T> normals;

        std::tie(surface, normals) = [&]
        {
                static constexpr std::optional<numerical::Vector<N, T>> GEOMETRIC_NORMAL;
                return scene_intersect<FLAT_SHADING, N, T, Color>(scene, GEOMETRIC_NORMAL, ray);
        }();

        T pdf_forward = pdf;

        for (int depth = 0; depth < MAX_DEPTH; ++depth)
        {
                if (!surface_found(ray, surface, normals))
                {
                        if (camera_path)
                        {
                                path->emplace_back(
                                        std::in_place_type<vertex::InfiniteLight<N, T, Color>>, &scene,
                                        &light_distribution, ray, beta, pdf_forward);
                        }
                        return;
                }

                const auto sample = surface_sample_with_pdf(surface, -ray.dir(), normals, engine);

                add_sample(sample, beta, pdf_forward, ray, surface, normals, path);

                if (!sample)
                {
                        return;
                }

                pdf_forward = sample->pdf_forward;
                beta *= sample->beta;

                if (!camera_path)
                {
                        beta *= correct_normals(normals, sample->l, -ray.dir());
                }

                if (beta.is_black())
                {
                        return;
                }

                ray = {surface.point(), sample->l};
                std::tie(surface, normals) = scene_intersect<FLAT_SHADING, N, T, Color>(scene, normals.geometric, ray);
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void generate_camera_path(
        const Scene<N, T, Color>& scene,
        const LightDistribution<N, T, Color>& light_distribution,
        const numerical::Ray<N, T>& ray,
        PCG& engine,
        std::vector<vertex::Vertex<N, T, Color>>* const path)
{
        path->clear();

        path->emplace_back(std::in_place_type<vertex::Camera<N, T, Color>>, ray.dir());

        walk<FLAT_SHADING>(
                /*camera_path=*/true, scene, light_distribution, /*beta=*/Color{1}, /*pdf=*/T{1}, ray, engine, path);

        ASSERT(path->size() >= 1);
        ASSERT(path->size() <= MAX_DEPTH + 1);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void generate_light_path(
        const Scene<N, T, Color>& scene,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine,
        std::vector<vertex::Vertex<N, T, Color>>* const path)
{
        path->clear();

        const LightDistributionSample distribution = light_distribution.sample(engine);
        const LightSourceLeaveSample sample = distribution.light->leave_sample(engine);

        if (!(sample.pdf_pos > 0 && sample.pdf_dir > 0 && !sample.radiance.is_black()))
        {
                return;
        }

        path->emplace_back(
                std::in_place_type<vertex::Light<N, T, Color>>, distribution.light,
                sample.infinite_distance ? std::optional<numerical::Vector<N, T>>() : sample.ray.org(),
                sample.ray.dir(), sample.n, distribution.pdf, sample.pdf_pos, sample.pdf_dir);

        const T pdf = distribution.pdf * sample.pdf_pos * sample.pdf_dir;
        const T k = sample.n ? std::max<T>(0, dot(*sample.n, sample.ray.dir())) : 1;
        const Color beta = sample.radiance * (k / pdf);

        walk<FLAT_SHADING>(
                /*camera_path=*/false, scene, light_distribution, beta, sample.pdf_dir, sample.ray, engine, path);

        ASSERT(path->size() <= MAX_DEPTH + 1);
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> bpt(
        const Scene<N, T, Color>& scene,
        const numerical::Ray<N, T>& ray,
        LightDistribution<N, T, Color>& light_distribution,
        PCG& engine)
{
        thread_local std::vector<vertex::Vertex<N, T, Color>> camera_path;
        thread_local std::vector<vertex::Vertex<N, T, Color>> light_path;

        generate_camera_path<FLAT_SHADING>(scene, light_distribution, ray, engine, &camera_path);

        if (camera_path.size() == 1)
        {
                return Color(0);
        }

        if (camera_path.size() == 2 && std::holds_alternative<vertex::InfiniteLight<N, T, Color>>(camera_path[1]))
        {
                return {};
        }

        generate_light_path<FLAT_SHADING>(scene, light_distribution, engine, &light_path);

        const int camera_size = camera_path.size();
        const int light_size = light_path.size();

        Color color(0);

        for (int t = 2; t <= camera_size; ++t)
        {
                for (int s = 0; s <= light_size; ++s)
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

#define TEMPLATE(N, T, C)                                                                                   \
        template std::optional<C> bpt<true, (N), T, C>(                                                     \
                const Scene<(N), T, C>&, const numerical::Ray<(N), T>&, LightDistribution<N, T, C>&, PCG&); \
        template std::optional<C> bpt<false, (N), T, C>(                                                    \
                const Scene<(N), T, C>&, const numerical::Ray<(N), T>&, LightDistribution<N, T, C>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
