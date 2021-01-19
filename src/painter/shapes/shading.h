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
 Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
 Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
 Real-Time Rendering. Fourth Edition.
 CRC Press, 2018.

 9.5 Fresnel Reflectance.
 9.6 Microgeometry.
 9.7 Microfacet Theory.
 9.8 BRDF Models for Surface Reflection.
 9.9 BRDF Models for Subsurface Scattering.
*/

/*
          F(h, l) G2(l, v, h) D(h)
 f spec = ------------------------   (9.34)
             4 |n · l| |n · v|
*/

#pragma once

#include "../objects.h"

#include <src/color/color.h>
#include <src/com/constant.h>
#include <src/com/interpolation.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>
#include <src/sampling/sphere_surface.h>
#include <src/sampling/sphere_uniform.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class Shading
{
        static_assert(N >= 4);

        static constexpr T DIFFUSE_REFLECTANCE = T(1) / sampling::sphere_integrate_cosine_factor_over_hemisphere(N);

public:
        static Color lighting(
                T /*metalness*/,
                T /*roughness*/,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& /*v*/,
                const Vector<N, T>& l)
        {
                return DIFFUSE_REFLECTANCE * dot(n, l) * color;
        }

        template <typename RandomEngine>
        static SurfaceReflection<N, T> reflection(
                RandomEngine& random_engine,
                T /*metalness*/,
                T /*roughness*/,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& /*v*/)
        {
                Vector<N, T> v = sampling::cosine_weighted_on_hemisphere(random_engine, n);
                return {DIFFUSE_REFLECTANCE * color, v};
        }
};

template <typename T>
class Shading<3, T>
{
        static constexpr std::size_t N = 3;

        inline static auto mix = [](const auto&... v)
        {
                return interpolation(v...);
        };

        static T sqr(T v)
        {
                return v * v;
        }

        // (9.16)
        // Schlick approximation of Fresnel reflectance
        static Color fresnel(const Color& f0, T h_l)
        {
                return mix(f0, Color(1), std::pow(1 - h_l, T(5)));
        }

        // (9.41)
        // GGX distribution
        static T d(T alpha_2, T n_h)
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
        Color diffuse(const Color& f0, const Color& color, T n_l, T n_v)
        {
                T l = (1 - std::pow(1 - n_l, T(5)));
                T v = (1 - std::pow(1 - n_v, T(5)));
                return (Color(1) - f0) * color * ((21 / (20 * PI<T>)) * l * v);
        }

        // (9.66), (9.67) without the subsurface term
        static Color diffuse_disney_without_subsurface(const Color& color, T roughness, T n_l, T n_v, T h_l)
        {
                T l = std::pow(1 - n_l, T(5));
                T v = std::pow(1 - n_v, T(5));
                T d_90 = T(0.5) + 2 * roughness * sqr(h_l);
                T f_d = mix(T(1), d_90, l) * mix(T(1), d_90, v);
                return color * (n_l * n_v * (1 / PI<T>)*f_d);
        }

        // (9.66), (9.67)
        static Color diffuse_disney(const Color& color, T roughness, T n_l, T n_v, T h_l, T k_ss)
        {
                T l = std::pow(1 - n_l, T(5));
                T v = std::pow(1 - n_v, T(5));
                T ss_90 = roughness * sqr(h_l);
                T d_90 = T(0.5) + 2 * ss_90;
                T f_d = mix(T(1), d_90, l) * mix(T(1), d_90, v);
                T f_ss = mix(T(1), ss_90, l) * mix(T(1), ss_90, v);
                f_ss = (1 / (n_l * n_v) - T(0.5)) * f_ss + T(0.5);
                return color * (n_l * n_v * (1 / PI<T>)*mix(f_d, T(1.25) * f_ss, k_ss));
        }

        static Color shade(
                T metalness,
                T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& l,
                const Vector<N, T>& v)
        {
                T n_l = dot(n, l);
                if (n_l <= 0)
                {
                        return Color(0);
                }

                constexpr T F0 = 0.05;

                const T alpha = sqr(roughness);
                const T alpha_2 = sqr(alpha);

                Color f0 = mix(Color(F0), color, metalness);
                Color diffuse_color = mix(color, Color(0), metalness);

                Vector<N, T> h = (l + v).normalized();
                T h_l = dot(h, l);
                T n_v = dot(n, v);
                T n_h = dot(n, h);

                Color spec = fresnel(f0, h_l) * g2_combined(alpha_2, n_l, n_v) * d(alpha_2, n_h);
                Color diff = diffuse_disney_without_subsurface(diffuse_color, roughness, n_l, n_v, h_l);

                return spec + diff;
        }

public:
        static Color lighting(
                T metalness,
                T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l)
        {
                return shade(metalness, roughness, color, n, l, v);
        }

        template <typename RandomEngine>
        static SurfaceReflection<N, T> reflection(
                RandomEngine& random_engine,
                T metalness,
                T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v)
        {
                Vector<N, T> l = sampling::uniform_on_sphere<N, T>(random_engine);
                if (dot(n, l) < 0)
                {
                        l = -l;
                }
                return {shade(metalness, roughness, color, n, l, v), l};
        }
};
}
