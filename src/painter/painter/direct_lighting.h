/*
Copyright (C) 2017-2021 Topological Manifold

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
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.3 The BRDF
Reflectance equation (9.3)
*/

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

13.10 Importance sampling
13.10.1 Multiple importance sampling
14.3.1 Estimating the direct lighting integral
*/

#pragma once

#include "normals.h"
#include "visibility.h"

#include "../objects.h"

#include <src/com/error.h>
#include <src/sampling/mis.h>

#include <optional>

namespace ns::painter
{
namespace direct_lighting_implementation
{
template <typename T>
T mis_heuristic(const int f_n, const T f_pdf, const int g_n, const T g_pdf)
{
        return sampling::mis::power_heuristic(f_n, f_pdf, g_n, g_pdf);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> sample_light_with_mis(
        const LightSource<N, T, Color>& light,
        const Scene<N, T, Color>& scene,
        const SurfacePoint<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        const Vector<N, T>& n = normals.shading;

        const LightSourceSample<N, T, Color> sample = light.sample(engine, surface.point());
        if (sample.pdf <= 0 || sample.radiance.is_black())
        {
                return {};
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return {};
        }

        if (occluded(scene, normals, Ray<N, T>(surface.point(), l), sample.distance))
        {
                return {};
        }

        const Color brdf = surface.brdf(n, v, l);
        if (light.is_delta())
        {
                return brdf * sample.radiance * (n_l / sample.pdf);
        }
        const T pdf = surface.pdf(n, v, l);
        const T weight = mis_heuristic(1, sample.pdf, 1, pdf);
        return brdf * sample.radiance * (weight * n_l / sample.pdf);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> sample_brdf_with_mis(
        const LightSource<N, T, Color>& light,
        const Scene<N, T, Color>& scene,
        const SurfacePoint<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        if (light.is_delta())
        {
                return {};
        }

        const Vector<N, T>& n = normals.shading;

        const Sample<N, T, Color> sample = surface.sample_brdf(engine, n, v);
        if (sample.pdf <= 0 || sample.brdf.is_black())
        {
                return {};
        }

        const Vector<N, T>& l = sample.l;
        ASSERT(l.is_unit());

        const T n_l = dot(n, l);
        if (n_l <= 0)
        {
                return {};
        }

        LightSourceInfo<T, Color> light_info = light.info(surface.point(), l);
        if (light_info.pdf <= 0 || light_info.radiance.is_black())
        {
                return {};
        }

        if (occluded(scene, normals, Ray<N, T>(surface.point(), l), light_info.distance))
        {
                return {};
        }

        if (sample.specular)
        {
                return sample.brdf * light_info.radiance * (n_l / sample.pdf);
        }
        const T weight = mis_heuristic(1, sample.pdf, 1, light_info.pdf);
        return sample.brdf * light_info.radiance * (weight * n_l / sample.pdf);
}

template <typename Dst, typename Src>
void add(Dst* const dst, Src&& src) requires requires
{
        **dst = src;
}
{
        if (*dst)
        {
                **dst += std::forward<Src>(src);
        }
        else
        {
                *dst = std::forward<Src>(src);
        }
};

template <typename Dst, typename Src>
void add(Dst* const dst, Src&& src) requires requires
{
        **dst = *src;
}
{
        if (src)
        {
                add(dst, *std::forward<Src>(src));
        }
}
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
std::optional<Color> direct_lighting(
        const Scene<N, T, Color>& scene,
        const SurfacePoint<N, T, Color>& surface,
        const Vector<N, T>& v,
        const Normals<N, T>& normals,
        RandomEngine& engine)
{
        namespace impl = direct_lighting_implementation;

        std::optional<Color> res;

        for (const LightSource<N, T, Color>* const light : scene.light_sources())
        {
                impl::add(&res, impl::sample_light_with_mis(*light, scene, surface, v, normals, engine));
                impl::add(&res, impl::sample_brdf_with_mis(*light, scene, surface, v, normals, engine));
        }

        return res;
}

template <std::size_t N, typename T, typename Color>
std::optional<Color> directly_visible_light_sources(
        const Scene<N, T, Color>& scene,
        const SurfacePoint<N, T, Color>& surface,
        const Ray<N, T>& ray)
{
        namespace impl = direct_lighting_implementation;

        std::optional<Color> res;

        for (const LightSource<N, T, Color>* const light : scene.light_sources())
        {
                LightSourceInfo<T, Color> light_info = light->info(ray.org(), ray.dir());

                if (light_info.pdf <= 0 || light_info.radiance.is_black())
                {
                        continue;
                }

                if (surface_before_distance(ray.org(), surface, light_info.distance))
                {
                        continue;
                }

                impl::add(&res, light_info.radiance);
        }

        return res;
}
}
