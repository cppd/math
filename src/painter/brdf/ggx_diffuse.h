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

9.5 Fresnel Reflectance
9.6 Microgeometry
9.7 Microfacet Theory
9.8 BRDF Models for Surface Reflection
9.9 BRDF Models for Subsurface Scattering

         F(h, l) G2(l, v, h) D(h)
f spec = ------------------------   (9.34)
            4 |n · l| |n · v|
*/

/*
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

13.10 Importance sampling
14.1.2 FresnelBlend
*/

#pragma once

#include "../objects.h"

#include <src/color/color.h>
#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/interpolation.h>
#include <src/com/math.h>
#include <src/numerical/optics.h>
#include <src/numerical/vec.h>
#include <src/sampling/ggx.h>
#include <src/sampling/sphere_cosine.h>

#include <cmath>

namespace ns::painter
{
template <typename T>
class GGXDiffuseBRDF
{
        static constexpr std::size_t N = 3;

        using RGB = Vector<3, T>;

        inline static const auto mix = [](const auto&... v)
        {
                return interpolation(v...);
        };

        static T sqr(T v)
        {
                return v * v;
        }

        // (9.16)
        // Schlick approximation of Fresnel reflectance
        static RGB fresnel(const RGB& f0, T h_l)
        {
                return mix(f0, RGB(1), power<5>(1 - h_l));
        }

        // (9.41)
        // GGX distribution
        static T ggx(T alpha_2, T n_h)
        {
                T v = 1 + sqr(n_h) * (alpha_2 - 1);
                return n_h * alpha_2 / (PI<T> * sqr(v));
        }

        // (9.43)
        // The combined term for GGX distribution
        // and the Smith masking-shadowing function
        //     G2(l, v)
        // -----------------
        // 4 |n · l| |n · v|
        static T g2_combined(T alpha_2, T n_l, T n_v)
        {
                T lv = n_l * std::sqrt(mix(sqr(n_v), T(1), alpha_2));
                T vl = n_v * std::sqrt(mix(sqr(n_l), T(1), alpha_2));
                return T(0.5) / (lv + vl);
        }

        // (9.64)
        RGB diffuse(const RGB& f0, const RGB& rho_ss, T n_l, T n_v)
        {
                T l = (1 - power<5>(1 - n_l));
                T v = (1 - power<5>(1 - n_v));
                return (RGB(1) - f0) * rho_ss * ((21 / (20 * PI<T>)) * l * v);
        }

        // (9.66), (9.67) without the subsurface term
        static RGB diffuse_disney_without_subsurface(const RGB& rho_ss, T roughness, T n_l, T n_v, T h_l)
        {
                T l = power<5>(1 - n_l);
                T v = power<5>(1 - n_v);
                T d_90 = T(0.5) + 2 * roughness * sqr(h_l);
                T f_d = mix(T(1), d_90, l) * mix(T(1), d_90, v);
                return rho_ss * (n_l * n_v * (1 / PI<T>)*f_d);
        }

        // (9.66), (9.67)
        static RGB diffuse_disney(const RGB& rho_ss, T roughness, T n_l, T n_v, T h_l, T k_ss)
        {
                T l = power<5>(1 - n_l);
                T v = power<5>(1 - n_v);
                T ss_90 = roughness * sqr(h_l);
                T d_90 = T(0.5) + 2 * ss_90;
                T f_d = mix(T(1), d_90, l) * mix(T(1), d_90, v);
                T f_ss = (1 / (n_l * n_v) - T(0.5)) * mix(T(1), ss_90, l) * mix(T(1), ss_90, v) + T(0.5);
                return rho_ss * (n_l * n_v * (1 / PI<T>)*mix(f_d, T(1.25) * f_ss, k_ss));
        }

        static RGB f(
                T metalness,
                T roughness,
                const RGB& surface_color,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l)
        {
                constexpr T F0 = 0.05;

                T alpha = sqr(roughness);
                T alpha_2 = sqr(alpha);

                RGB f0 = mix(RGB(F0), surface_color, metalness);
                RGB rho_ss = mix(surface_color, RGB(0), metalness);

                Vector<N, T> h = (l + v).normalized();

                T n_l = dot(n, l);
                T h_l = dot(h, l);
                T n_v = dot(n, v);
                T n_h = dot(n, h);

                RGB spec = fresnel(f0, h_l) * g2_combined(alpha_2, n_l, n_v) * ggx(alpha_2, n_h);
                RGB diff = diffuse_disney_without_subsurface(rho_ss, roughness, n_l, n_v, h_l);

                return spec + diff;
        }

        template <typename RandomEngine>
        static std::tuple<Vector<N, T>, T> sample_cosine(RandomEngine& random_engine, const Vector<N, T>& n)
        {
                Vector<N, T> l = sampling::cosine_on_hemisphere(random_engine, n);
                ASSERT(l.is_unit());
                if (dot(n, l) <= 0)
                {
                        return {Vector<N, T>(0), 0};
                }
                T pdf = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
                return {l, pdf};
        }

        template <typename RandomEngine>
        static std::tuple<Vector<N, T>, T> sample_ggx_cosine(
                RandomEngine& random_engine,
                T roughness,
                const Vector<N, T>& n,
                const Vector<N, T>& v)
        {
                // 14.1.2 FresnelBlend
                // Sample from both a cosine-weighted distribution
                // as well as the microfacet distribution.
                // The PDF is an average of the two PDFs used.

                const T alpha = sqr(roughness);

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
                        h = sampling::ggx_vn(random_engine, n, v, alpha);
                        l = numerical::reflect_vn(v, h);
                        ASSERT(l.is_unit());
                        if (dot(n, l) <= 0)
                        {
                                return {Vector<N, T>(0), 0};
                        }
                }

                T pdf_cosine = sampling::cosine_on_hemisphere_pdf<N>(dot(n, l));
                T pdf_ggx = sampling::ggx_vn_reflected_pdf<N>(dot(n, v), dot(n, h), dot(h, l), alpha);

                T pdf = T(0.5) * (pdf_cosine + pdf_ggx);

                return {l, pdf};
        }

public:
        static Color f(
                T metalness,
                T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l)
        {
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

                RGB s = f(metalness, roughness, color.rgb<T>(), n, v, l);
                return Color(s[0], s[1], s[2]);
        }

        template <typename RandomEngine>
        static BrdfSample<N, T> sample_f(
                RandomEngine& random_engine,
                T metalness,
                T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v)
        {
                static constexpr BrdfSample<N, T> BLACK(Vector<N, T>(0), 0, Color(0));

                ASSERT(n.is_unit());
                ASSERT(v.is_unit());

                if (dot(n, v) <= 0)
                {
                        return BLACK;
                }

                const auto [l, pdf] = sample_ggx_cosine(random_engine, roughness, n, v);
                if (pdf <= 0)
                {
                        return BLACK;
                }

                ASSERT(l.is_unit());
                ASSERT(dot(n, l) > 0);

                RGB s = f(metalness, roughness, color.rgb<T>(), n, v, l);
                return {l, pdf, Color(s[0], s[1], s[2])};
        }
};
}
