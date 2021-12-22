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

9.9 BRDF Models for Subsurface Scattering
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
#include "sample.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_cosine.h>

#include <cmath>

namespace ns::shading::ggx_diffuse
{
namespace implementation
{
// (9.64)
// template <std::size_t N, typename T, typename Color>
// Color diffuse(const Color& f0, const Color& rho_ss, const T n_l, const T n_v)
// {
//         static constexpr Color WHITE = Color(1);
//         static constexpr T K = geometry::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, T>;
//
//         const T l = (1 - power<5>(1 - n_l));
//         const T v = (1 - power<5>(1 - n_v));
//         const T c = (21 / (20 * K)) * l * v;
//         return c * (WHITE - f0) * rho_ss;
// }

// (9.66), (9.67) without the subsurface term
// template <std::size_t N, typename T, typename Color>
// Color diffuse_disney_ws(
//         const Color& /*f0*/,
//         const Color& rho_ss,
//         const T roughness,
//         const T n_l,
//         const T n_v,
//         const T h_l)
// {
//         static constexpr T K = 1 / geometry::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;
//
//         const T l = power<5>(1 - n_l);
//         const T v = power<5>(1 - n_v);
//         const T f_d90 = T(0.5) + 2 * roughness * square(h_l);
//         const T c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         return (c * K) * rho_ss;
// }
template <std::size_t N, typename T, typename Color>
Color diffuse_disney_ws(const Color& f0, const Color& rho_ss, const T roughness, const T n_l, const T n_v, const T h_l)
{
        static constexpr Color WHITE = Color(1);
        static constexpr T K = 1 / geometry::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;

        const T l = power<5>(1 - n_l);
        const T v = power<5>(1 - n_v);
        const T f_d90 = 2 * roughness * square(h_l);
        const T c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
        return (c * K) * (WHITE - f0) * rho_ss;
}
// (9.66), (9.67)
// template <std::size_t N, typename T, typename Color>
// Color diffuse_disney(const Color& rho_ss, const T roughness, const T n_l, const T n_v, const T h_l, const T k_ss)
// {
//         static constexpr T K = 1 / geometry::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;
//
//         const T l = power<5>(1 - n_l);
//         const T v = power<5>(1 - n_v);
//         const T f_ss90 = roughness * square(h_l);
//         const T f_d90 = T(0.5) + 2 * f_ss90;
//         const T f_d = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         const T f_ss = (1 / (n_l * n_v) - T(0.5)) * (1 + (f_ss90 - 1) * l) * (1 + (f_ss90 - 1) * v) + T(0.5);
//         const T c = interpolation(f_d, T(1.25) * f_ss, k_ss);
//         return (c * K) * rho_ss;
// }

template <bool GGX_ONLY, std::size_t N, typename T, typename Color>
Color f(const T metalness,
        const T roughness,
        const Color& surface_color,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l)
{
        const Vector<N, T> h = (l + v).normalized();

        const T n_l = dot(n, l);
        const T h_l = dot(h, l);
        const T n_v = dot(n, v);
        const T n_h = dot(n, h);

        static constexpr Color F0(0.05);
        const Color f0 = interpolation(F0, surface_color, metalness);
        const Color ggx = ggx_brdf<N>(roughness, f0, n_v, n_l, n_h, h_l);

        if (GGX_ONLY)
        {
                return ggx;
        }

        static constexpr Color BLACK(0);
        const Color rho_ss = interpolation(surface_color, BLACK, metalness);
        const Color diffuse = diffuse_disney_ws<N>(f0, rho_ss, roughness, n_l, n_v, h_l);

        return ggx + diffuse;
}

// template <std::size_t N, typename T, typename RandomEngine>
// std::tuple<Vector<N, T>, T> sample_cosine(RandomEngine& random_engine, const Vector<N, T>& n)
// {
//         const Vector<N, T> l = sampling::cosine_on_hemisphere(random_engine, n);
//         ASSERT(l.is_unit());
//         if (dot(n, l) <= 0)
//         {
//                 return {Vector<N, T>(0), 0};
//         }
//         const T pdf = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
//         return {l, pdf};
// }

template <bool GGX_ONLY, std::size_t N, typename T>
T pdf_ggx_cosine(
        const T alpha,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l,
        const Vector<N, T>& h)
{
        const T pdf_ggx = ggx_visible_normals_l_pdf<N>(dot(n, v), dot(n, h), dot(h, l), alpha);

        if (GGX_ONLY)
        {
                return pdf_ggx;
        }

        const T pdf_cosine = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));

        return T(0.5) * (pdf_cosine + pdf_ggx);
}

template <bool GGX_ONLY, std::size_t N, typename T, typename RandomEngine>
std::tuple<Vector<N, T>, T> sample_ggx_cosine(
        RandomEngine& random_engine,
        const T roughness,
        const Vector<N, T>& n,
        const Vector<N, T>& v)
{
        // 14.1.2 FresnelBlend
        // Sample from both a cosine-weighted distribution
        // as well as the microfacet distribution.
        // The PDF is an average of the two PDFs used.

        const T alpha = square(roughness);

        Vector<N, T> l;
        Vector<N, T> h;
        if (GGX_ONLY || std::bernoulli_distribution(0.5)(random_engine))
        {
                std::tie(h, l) = ggx_visible_normals_h_l(random_engine, n, v, alpha);
                ASSERT(l.is_unit());
                ASSERT(h.is_unit());
        }
        else
        {
                l = sampling::cosine_on_hemisphere(random_engine, n);
                ASSERT(l.is_unit());
                h = (v + l).normalized();
        }

        const T pdf = pdf_ggx_cosine<GGX_ONLY>(alpha, n, v, l, h);

        return {l, pdf};
}
}

//

template <bool GGX_ONLY = false, std::size_t N, typename T, typename Color>
Color f(const T metalness,
        const T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l)
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

        return impl::f<GGX_ONLY>(metalness, roughness, color, n, v, l);
}

template <bool GGX_ONLY = false, std::size_t N, typename T>
T pdf(const T roughness, const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l)
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
        RandomEngine& random_engine,
        const T metalness,
        const T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        ASSERT(n.is_unit());
        ASSERT(v.is_unit());

        if (dot(n, v) <= 0)
        {
                return {Vector<N, T>(0), 0, Color(0)};
        }

        const auto [l, pdf] = impl::sample_ggx_cosine<GGX_ONLY>(random_engine, roughness, n, v);

        if (pdf <= 0)
        {
                return {Vector<N, T>(0), 0, Color(0)};
        }

        ASSERT(l.is_unit());

        if (dot(n, l) <= 0)
        {
                return {l, pdf, Color(0)};
        }

        return {l, pdf, impl::f<GGX_ONLY>(metalness, roughness, color, n, v, l)};
}
}
