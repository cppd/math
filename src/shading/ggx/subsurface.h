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
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.9 BRDF Models for Subsurface Scattering
*/

#pragma once

#include <src/com/exponent.h>
#include <src/geometry/shapes/sphere_integral.h>

#include <cstddef>

namespace ns::shading::ggx
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
//         const T f_d90 = T{0.5} + 2 * roughness * square(h_l);
//         const T c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         return (c * K) * rho_ss;
// }
template <std::size_t N, typename T, typename Color>
Color diffuse_disney_ws(const Color& f0, const Color& rho_ss, const T roughness, const T n_l, const T n_v, const T h_l)
{
        static constexpr Color WHITE = Color(1);
        static constexpr T K = 1 / geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, long double>;

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
//         const T f_d90 = T{0.5} + 2 * f_ss90;
//         const T f_d = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         const T f_ss = (1 / (n_l * n_v) - T{0.5}) * (1 + (f_ss90 - 1) * l) * (1 + (f_ss90 - 1) * v) + T{0.5};
//         const T c = interpolation(f_d, T{1.25} * f_ss, k_ss);
//         return (c * K) * rho_ss;
// }
}
