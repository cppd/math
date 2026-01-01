/*
Copyright (C) 2017-2026 Topological Manifold

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

#ifndef SHADING_GGX_SUBSURFACE_GLSL
#define SHADING_GGX_SUBSURFACE_GLSL

#include "constants.glsl"

// (9.64)
// vec3 shading_ggx_diffuse(const vec3 f0, const vec3 rho_ss, const float n_l, const float n_v)
// {
//         const float l = (1 - pow(1 - n_l, 5));
//         const float v = (1 - pow(1 - n_v, 5));
//         const float c = (21 / (20 * SHADING_GGX_PI)) * l * v;
//         return c * (1 - f0) * rho_ss;
// }

// (9.66), (9.67) without the subsurface term
// vec3 shading_ggx_diffuse_disney_ws(
//         const vec3 /*f0*/,
//         const vec3 rho_ss,
//         const float roughness,
//         const float n_l,
//         const float n_v,
//         const float h_l)
// {
//         float l = pow(1 - n_l, 5);
//         float v = pow(1 - n_v, 5);
//         float f_d90 = 0.5 + 2 * roughness * pow(h_l, 2);
//         float c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         return (c * SHADING_GGX_PI_R) * rho_ss;
// }
vec3 shading_ggx_diffuse_disney_ws(
        const vec3 f0,
        const vec3 rho_ss,
        const float roughness,
        const float n_l,
        const float n_v,
        const float h_l)
{
        const float l = pow(1 - n_l, 5);
        const float v = pow(1 - n_v, 5);
        const float f_d90 = 2 * roughness * pow(h_l, 2);
        const float c = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
        return (c * SHADING_GGX_PI_R) * (1 - f0) * rho_ss;
}

// (9.66), (9.67)
// vec3 shading_ggx_diffuse_disney(
//         const vec3 rho_ss,
//         const float roughness,
//         const float n_l,
//         const float n_v,
//         const float h_l,
//         const float k_ss)
// {
//         const float l = pow(1 - n_l, 5);
//         const float v = pow(1 - n_v, 5);
//         const float f_ss90 = roughness * pow(h_l, 2);
//         const float f_d90 = 0.5 + 2 * f_ss90;
//         const float f_d = (1 + (f_d90 - 1) * l) * (1 + (f_d90 - 1) * v);
//         const float f_ss = (1 / (n_l * n_v) - 0.5) * (1 + (f_ss90 - 1) * l) * (1 + (f_ss90 - 1) * v) + 0.5;
//         const float c = mix(f_d, 1.25 * f_ss, k_ss);
//         return (c * SHADING_GGX_PI_R) * rho_ss;
// }

#endif
