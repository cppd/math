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
#include <src/geometry/shapes/sphere_surface.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>
#include <src/sampling/sphere_uniform.h>

namespace ns::painter
{
template <std::size_t N, typename T>
class Shading
{
        static_assert(N >= 4);

        static constexpr T CONSTANT_REFLECTANCE_FACTOR =
                T(1) / geometry::sphere_integrate_cosine_factor_over_hemisphere(N);

public:
        static Color direct_lighting(
                T /*metalness*/,
                T /*roughness*/,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& /*v*/,
                const Vector<N, T>& l)
        {
                // f = color / (integrate dot(n,l) over hemisphere)
                // pdf = 1
                // s = f / pdf * dot(n,l)
                // s = color / (integrate dot(n,l) over hemisphere) * dot(n,l)
                return CONSTANT_REFLECTANCE_FACTOR * dot(n, l) * color;
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
                // f = color / (integrate dot(n,l) over hemisphere)
                // pdf = dot(n,l) / (integrate dot(n,l) over hemisphere)
                // s = f / pdf * dot(n,l)
                // s = color / (integrate dot(n,l) over hemisphere) /
                //     (dot(n,l) / (integrate dot(n,l) over hemisphere)) * dot(n,l)
                // s = color
                return {color, v};
        }
};

template <typename T>
class Shading<3, T>
{
        static constexpr std::size_t N = 3;

        using vec3 = Vector<3, T>;
        static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(std::declval<vec3>()[0])>>);

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
        static vec3 fresnel(const vec3& f0, T h_l)
        {
                return mix(f0, vec3(1), std::pow(1 - h_l, T(5)));
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
        vec3 diffuse(const vec3& f0, const vec3& rho_ss, T n_l, T n_v)
        {
                T l = (1 - std::pow(1 - n_l, T(5)));
                T v = (1 - std::pow(1 - n_v, T(5)));
                return (vec3(1) - f0) * rho_ss * ((21 / (20 * PI<T>)) * l * v);
        }

        // (9.66), (9.67) without the subsurface term
        static vec3 diffuse_disney_without_subsurface(const vec3& rho_ss, T roughness, T n_l, T n_v, T h_l)
        {
                T l = std::pow(1 - n_l, T(5));
                T v = std::pow(1 - n_v, T(5));
                T d_90 = T(0.5) + 2 * roughness * sqr(h_l);
                T f_d = mix(T(1), d_90, l) * mix(T(1), d_90, v);
                return rho_ss * (n_l * n_v * (1 / PI<T>)*f_d);
        }

        // (9.66), (9.67)
        static vec3 diffuse_disney(const vec3& rho_ss, T roughness, T n_l, T n_v, T h_l, T k_ss)
        {
                T l = std::pow(1 - n_l, T(5));
                T v = std::pow(1 - n_v, T(5));
                T ss_90 = roughness * sqr(h_l);
                T d_90 = T(0.5) + 2 * ss_90;
                T f_d = mix(T(1), d_90, l) * mix(T(1), d_90, v);
                T f_ss = (1 / (n_l * n_v) - T(0.5)) * mix(T(1), ss_90, l) * mix(T(1), ss_90, v) + T(0.5);
                return rho_ss * (n_l * n_v * (1 / PI<T>)*mix(f_d, T(1.25) * f_ss, k_ss));
        }

        static vec3 f(
                T metalness,
                T roughness,
                const vec3& surface_color,
                T n_l,
                const Vector<N, T>& n,
                const Vector<N, T>& l,
                const Vector<N, T>& v)
        {
                constexpr T F0 = 0.05;

                T alpha = sqr(roughness);
                T alpha_2 = sqr(alpha);

                vec3 f0 = mix(vec3(F0), surface_color, metalness);
                vec3 rho_ss = mix(surface_color, vec3(0), metalness);

                Vector<N, T> h = (l + v).normalized();

                T h_l = dot(h, l);
                T n_v = dot(n, v);
                T n_h = dot(n, h);

                vec3 spec = fresnel(f0, h_l) * g2_combined(alpha_2, n_l, n_v) * ggx(alpha_2, n_h);
                vec3 diff = diffuse_disney_without_subsurface(rho_ss, roughness, n_l, n_v, h_l);

                return spec + diff;
        }

public:
        static Color direct_lighting(
                T metalness,
                T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l)
        {
                T n_l = dot(n, l);
                if (n_l <= 0)
                {
                        return Color(0);
                }
                // pdf = 1
                // s = f / pdf * dot(n,l)
                // s = f * dot(n,l)
                vec3 s = n_l * f(metalness, roughness, color.to_rgb_vector<T>(), n_l, n, l, v);
                return Color(to_vector<Color::DataType>(s));
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
                Vector<N, T> l = sampling::cosine_weighted_on_hemisphere(random_engine, n);
                T n_l = dot(n, l);
                // pdf = dot(n,l) / (integrate dot(n,l) over 2-hemisphere)
                // pdf = dot(n,l) / PI
                // s = f / pdf * dot(n,l)
                // s = f / (dot(n,l) / PI) * dot(n,l)
                // s = f * PI
                vec3 s = PI<T> * f(metalness, roughness, color.to_rgb_vector<T>(), n_l, n, l, v);
                return {Color(to_vector<Color::DataType>(s)), l};
        }
};
}
