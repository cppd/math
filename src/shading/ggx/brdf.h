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

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

13.10 Importance sampling
14.1.2 FresnelBlend
*/

#pragma once

#include "ggx.h"
#include "multiple_bounce.h"
#include "subsurface.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_cosine.h>
#include <src/shading/objects.h>

#include <cstddef>
#include <random>
#include <tuple>

namespace ns::shading::ggx::brdf
{
namespace implementation
{
template <bool GGX_ONLY, std::size_t N, typename T, typename Color>
Color f(const T roughness,
        const Color& f0,
        const Color& rho_ss,
        const numerical::Vector<N, T>& n,
        const numerical::Vector<N, T>& v,
        const numerical::Vector<N, T>& l)
{
        const numerical::Vector<N, T> h = (l + v).normalized();

        const T n_l = dot(n, l);
        const T h_l = dot(h, l);
        const T n_v = dot(n, v);
        const T n_h = dot(n, h);

        const Color ggx = ggx_brdf<N>(roughness, f0, n_v, n_l, n_h, h_l);

        if (GGX_ONLY)
        {
                return ggx;
        }

        const Color multiple_bounce = multiple_bounce_surface_reflection<N>(f0, roughness, n_l, n_v);

        const Color diffuse = diffuse_disney_ws<N>(f0, rho_ss, roughness, n_l, n_v, h_l);

        return ggx + multiple_bounce + diffuse;
}

// template <std::size_t N, typename T, typename RandomEngine>
// std::tuple<numerical::Vector<N, T>, T> sample_cosine(RandomEngine& engine, const numerical::Vector<N, T>& n)
// {
//         const numerical::Vector<N, T> l = sampling::cosine_on_hemisphere(engine, n);
//         ASSERT(l.is_unit());
//         if (dot(n, l) <= 0)
//         {
//                 return {numerical::Vector<N, T>(0), 0};
//         }
//         const T pdf = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
//         return {l, pdf};
// }

template <bool GGX_ONLY, std::size_t N, typename T>
T pdf_ggx_cosine(
        const T alpha,
        const numerical::Vector<N, T>& n,
        const numerical::Vector<N, T>& v,
        const numerical::Vector<N, T>& l,
        const numerical::Vector<N, T>& h)
{
        const T pdf_ggx = ggx_visible_normals_l_pdf<N>(dot(n, v), dot(n, h), dot(h, l), alpha);

        if (GGX_ONLY)
        {
                return pdf_ggx;
        }

        const T pdf_cosine = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));

        return T{0.5} * (pdf_cosine + pdf_ggx);
}

template <bool GGX_ONLY, std::size_t N, typename T, typename RandomEngine>
std::tuple<numerical::Vector<N, T>, T> sample_ggx_cosine(
        RandomEngine& engine,
        const T roughness,
        const numerical::Vector<N, T>& n,
        const numerical::Vector<N, T>& v)
{
        // 14.1.2 FresnelBlend
        // Sample from both a cosine-weighted distribution
        // as well as the microfacet distribution.
        // The PDF is an average of the two PDFs used.

        const T alpha = square(roughness);

        numerical::Vector<N, T> l;
        numerical::Vector<N, T> h;
        if (GGX_ONLY || std::bernoulli_distribution(0.5)(engine))
        {
                std::tie(h, l) = ggx_visible_normals_h_l(engine, n, v, alpha);
                ASSERT(l.is_unit());
                ASSERT(h.is_unit());
        }
        else
        {
                l = sampling::cosine_on_hemisphere(engine, n);
                ASSERT(l.is_unit());
                h = (v + l).normalized();
        }

        const T pdf = pdf_ggx_cosine<GGX_ONLY>(alpha, n, v, l, h);

        return {l, pdf};
}
}

//

template <bool GGX_ONLY = false, std::size_t N, typename T, typename Color>
Color f(const T roughness,
        const Colors<Color>& colors,
        const numerical::Vector<N, T>& n,
        const numerical::Vector<N, T>& v,
        const numerical::Vector<N, T>& l)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        ASSERT(n.is_unit());
        ASSERT(v.is_unit());
        ASSERT(l.is_unit());

        if (dot(n, v) <= 0)
        {
                return Color(0);
        }

        if (dot(n, l) <= 0)
        {
                return Color(0);
        }

        return impl::f<GGX_ONLY>(roughness, colors.f0, colors.rho_ss, n, v, l);
}

template <bool GGX_ONLY = false, std::size_t N, typename T>
T pdf(const T roughness,
      const numerical::Vector<N, T>& n,
      const numerical::Vector<N, T>& v,
      const numerical::Vector<N, T>& l)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        ASSERT(n.is_unit());
        ASSERT(v.is_unit());
        ASSERT(l.is_unit());

        if (dot(n, v) <= 0)
        {
                return 0;
        }

        const T alpha = square(roughness);

        return impl::pdf_ggx_cosine<GGX_ONLY>(alpha, n, v, l, (v + l).normalized());
}

template <bool GGX_ONLY = false, std::size_t N, typename T, typename Color, typename RandomEngine>
Sample<N, T, Color> sample_f(
        RandomEngine& engine,
        const T roughness,
        const Colors<Color>& colors,
        const numerical::Vector<N, T>& n,
        const numerical::Vector<N, T>& v)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        ASSERT(n.is_unit());
        ASSERT(v.is_unit());

        if (dot(n, v) <= 0)
        {
                return {numerical::Vector<N, T>(0), 0, Color(0)};
        }

        const auto [l, pdf] = impl::sample_ggx_cosine<GGX_ONLY>(engine, roughness, n, v);

        if (pdf <= 0)
        {
                return {numerical::Vector<N, T>(0), 0, Color(0)};
        }

        ASSERT(l.is_unit());

        if (dot(n, l) <= 0)
        {
                return {l, pdf, Color(0)};
        }

        return {l, pdf, impl::f<GGX_ONLY>(roughness, colors.f0, colors.rho_ss, n, v, l)};
}
}
