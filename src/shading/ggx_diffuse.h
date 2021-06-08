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

#include "sample.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/interpolation.h>
#include <src/com/math.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>
#include <src/shading/ggx.h>

#include <cmath>

namespace ns::shading::ggx_diffuse
{
namespace implementation
{
// (9.64)
template <std::size_t N, typename T, typename Color>
Color diffuse(const Color& f0, const Color& rho_ss, T n_l, T n_v)
{
        static constexpr Color WHITE = Color(1);
        static constexpr T K = geometry::sphere_integrate_cosine_factor_over_hemisphere(N);

        T l = (1 - power<5>(1 - n_l));
        T v = (1 - power<5>(1 - n_v));
        T c = (21 / (20 * K)) * l * v;

        return c * (WHITE - f0) * rho_ss;
}

// (9.66), (9.67) without the subsurface term
//template <typename T, typename Color>
//Color diffuse_disney_without_subsurface(const Color& rho_ss, T roughness, T n_l, T n_v, T h_l)
//{
//        T l = power<5>(1 - n_l);
//        T v = power<5>(1 - n_v);
//        T f_d90 = T(0.5) + 2 * roughness * square(h_l);
//        T f_d90_1 = f_d90 - 1;
//        T f_d = (1 + f_d90_1 * l) * (1 + f_d90_1 * v);
//        T c = f_d / PI<T>;
//        return c * rho_ss;
//}

// (9.66), (9.67)
//template <typename T, typename Color>
//Color diffuse_disney(const Color& rho_ss, T roughness, T n_l, T n_v, T h_l, T k_ss)
//{
//        T l = power<5>(1 - n_l);
//        T v = power<5>(1 - n_v);
//        T f_ss90 = roughness * square(h_l);
//        T f_d90 = T(0.5) + 2 * f_ss90;
//        T f_d90_1 = f_d90 - 1;
//        T f_d = (1 + f_d90_1 * l) * (1 + f_d90_1 * v);
//        T f_ss90_1 = f_ss90 - 1;
//        T f_ss = (1 / (n_l * n_v) - T(0.5)) * (1 + f_ss90_1 * l) * (1 + f_ss90_1 * v) + T(0.5);
//        T c = interpolation(f_d, T(1.25) * f_ss, k_ss) / PI<T>;
//        return c * rho_ss;
//}

template <std::size_t N, typename T, typename Color>
Color f(T metalness,
        T roughness,
        const Color& surface_color,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l)
{
        Vector<N, T> h = (l + v).normalized();

        T n_l = dot(n, l);
        T h_l = dot(h, l);
        T n_v = dot(n, v);
        T n_h = dot(n, h);

        static constexpr Color F0(0.05);
        static constexpr Color BLACK(0);
        const Color f0 = interpolation(F0, surface_color, metalness);
        const Color rho_ss = interpolation(surface_color, BLACK, metalness);

        Color spec = ggx_brdf<N>(roughness, f0, n_v, n_l, n_h, h_l);
        Color diff = diffuse<N>(f0, rho_ss, n_l, n_v);

        return spec + diff;
}

//template <std::size_t N, typename T, typename RandomEngine>
//std::tuple<Vector<N, T>, T> sample_cosine(RandomEngine& random_engine, const Vector<N, T>& n)
//{
//        Vector<N, T> l = sampling::cosine_on_hemisphere(random_engine, n);
//        ASSERT(l.is_unit());
//        if (dot(n, l) <= 0)
//        {
//                return {Vector<N, T>(0), 0};
//        }
//        T pdf = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
//        return {l, pdf};
//}

template <std::size_t N, typename T, typename RandomEngine>
std::tuple<Vector<N, T>, T> sample_ggx_cosine(
        RandomEngine& random_engine,
        T roughness,
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
        if (std::bernoulli_distribution(0.5)(random_engine))
        {
                l = sampling::cosine_on_hemisphere(random_engine, n);
                ASSERT(l.is_unit());
                if (dot(n, l) <= 0)
                {
                        return {Vector<N, T>(0), 0};
                }
                h = (v + l).normalized();
        }
        else
        {
                std::tie(h, l) = ggx_visible_normals_h_l(random_engine, n, v, alpha);
                ASSERT(l.is_unit());
                if (dot(n, l) <= 0)
                {
                        return {Vector<N, T>(0), 0};
                }
                ASSERT(h.is_unit());
        }

        T pdf_cosine = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
        T pdf_ggx = ggx_visible_normals_l_pdf<N>(dot(n, v), dot(n, h), dot(h, l), alpha);

        T pdf = T(0.5) * (pdf_cosine + pdf_ggx);

        return {l, pdf};
}
}

//

template <std::size_t N, typename T, typename Color>
Color f(T metalness,
        T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        static constexpr Color BLACK(0);

        ASSERT(n.is_unit());
        ASSERT(v.is_unit());
        ASSERT(l.is_unit());

        if (dot(n, v) <= 0)
        {
                return BLACK;
        }
        if (dot(n, l) <= 0)
        {
                return BLACK;
        }

        return impl::f(metalness, roughness, color, n, v, l);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
Sample<N, T, Color> sample_f(
        RandomEngine& random_engine,
        T metalness,
        T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v)
{
        static_assert(N >= 3);
        namespace impl = implementation;

        static constexpr Sample<N, T, Color> BLACK(Vector<N, T>(0), 0, Color(0));

        ASSERT(n.is_unit());
        ASSERT(v.is_unit());

        if (dot(n, v) <= 0)
        {
                return BLACK;
        }

        const auto [l, pdf] = impl::sample_ggx_cosine(random_engine, roughness, n, v);
        if (pdf <= 0)
        {
                return BLACK;
        }

        ASSERT(l.is_unit());
        ASSERT(dot(n, l) > 0);

        return {l, pdf, impl::f(metalness, roughness, color, n, v, l)};
}
}
