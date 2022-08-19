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

#include "normals.h"
#include "surface_sample.h"
#include "visibility.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/sampling/pdf.h>
#include <src/settings/instantiation.h>

#include <cmath>

namespace ns::painter::integrators
{
namespace
{
constexpr int MAX_DEPTH = 5;

template <std::size_t N, typename T, typename Color>
struct Vertex final
{
        Vector<N, T> pos;
        Vector<N, T> normal;
        Color beta;
        T pdf_forward = 0;
        T pdf_reversed = 0;
};

template <std::size_t N, typename T>
T solid_angle_pdf_to_area_pdf(
        const Vector<N, T>& prev_pos,
        const T angle_pdf,
        const Vector<N, T>& next_pos,
        const Vector<N, T>& next_normal)
{
        const Vector<N, T> v = prev_pos - next_pos;
        const T distance = v.norm();
        const T cosine = std::abs(dot(v, next_normal)) / distance;
        return sampling::solid_angle_pdf_to_area_pdf<N, T>(angle_pdf, cosine, distance);
}

template <std::size_t N, typename T, typename Color>
bool surface_found(const Ray<N, T>& ray, const SurfaceIntersection<N, T, Color>& surface, const Normals<N, T>& normals)
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
                path->push_back({.pos = surface.point(), .normal = normals.shading, .beta = beta});

                Vertex<N, T, Color>& prev = *(path->end() - 2);
                Vertex<N, T, Color>& next = *(path->end() - 1);

                next.pdf_forward = solid_angle_pdf_to_area_pdf(prev.pos, pdf_forward, next.pos, next.normal);

                const auto sample = surface_sample_bd(surface, -ray.dir(), normals, engine);
                if (!sample)
                {
                        break;
                }

                prev.pdf_reversed = solid_angle_pdf_to_area_pdf(next.pos, sample->pdf_reversed, prev.pos, prev.normal);

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

        path->push_back({.pos = ray.org(), .normal = ray.dir(), .beta = beta, .pdf_forward = 1, .pdf_reversed = 0});

        walk<FLAT_SHADING>(scene, beta, T{1}, ray, engine, path);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void generate_light_path(
        const Scene<N, T, Color>& scene,
        LightDistribution<T>& light_distribution,
        PCG& engine,
        std::vector<Vertex<N, T, Color>>* const path)
{
        path->clear();

        const LightDistributionSample light_distribution_sample = light_distribution.sample(engine);
        const LightSource<N, T, Color>* const light = scene.light_sources()[light_distribution_sample.index];
        const LightSourceSampleEmit light_sample = light->sample_emit(engine);

        if (!(light_sample.pdf_pos > 0 && light_sample.pdf_dir > 0 && !light_sample.radiance.is_black()))
        {
                return;
        }

        path->push_back(
                {.pos = light_sample.ray.org(),
                 .normal = light_sample.n ? *light_sample.n : Vector<N, T>(0),
                 .beta = light_sample.radiance,
                 .pdf_forward = light_distribution_sample.pdf * light_sample.pdf_pos,
                 .pdf_reversed = 0});

        const T pdf = light_distribution_sample.pdf * light_sample.pdf_pos * light_sample.pdf_dir;
        const T k = light_sample.n ? std::abs(dot(*light_sample.n, light_sample.ray.dir())) : 1;
        const Color beta = light_sample.radiance * (k / pdf);

        walk<FLAT_SHADING>(scene, beta, light_sample.pdf_dir, light_sample.ray, engine, path);
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::optional<Color> bpt(
        const Scene<N, T, Color>& scene,
        const Ray<N, T>& ray,
        LightDistribution<T>& light_distribution,
        PCG& engine)
{
        thread_local std::vector<Vertex<N, T, Color>> camera_path;
        thread_local std::vector<Vertex<N, T, Color>> light_path;

        generate_camera_path<FLAT_SHADING>(scene, ray, engine, &camera_path);
        generate_light_path<FLAT_SHADING>(scene, light_distribution, engine, &light_path);

        return {};
}

#define TEMPLATE(N, T, C)                                                                  \
        template std::optional<C> bpt<true, (N), T, C>(                                    \
                const Scene<(N), T, C>&, const Ray<(N), T>&, LightDistribution<T>&, PCG&); \
        template std::optional<C> bpt<false, (N), T, C>(                                   \
                const Scene<(N), T, C>&, const Ray<(N), T>&, LightDistribution<T>&, PCG&);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
