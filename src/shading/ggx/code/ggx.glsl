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

9.6 Microgeometry
9.7 Microfacet Theory
9.8 BRDF Models for Surface Reflection
*/

#ifndef SHADING_GGX_GGX_GLSL
#define SHADING_GGX_GGX_GLSL

#include "constants.glsl"
#include "fresnel.glsl"

// (9.41)
// GGX distribution
float shading_ggx_d(const float alpha_2, const float n_h)
{
        const float v = 1 + pow(n_h, 2) * (alpha_2 - 1);

        return alpha_2 / (SHADING_GGX_PI * pow(v, 2));
}

// (9.43)
// The combined term for GGX distribution
// and the Smith masking-shadowing function
//     G2(l, v)
// -----------------
// 4 |n · l| |n · v|
float shading_ggx_g2_combined(const float alpha_2, const float n_l, const float n_v)
{
        const float lv = n_l * sqrt(alpha_2 + pow(n_v, 2) * (1 - alpha_2));
        const float vl = n_v * sqrt(alpha_2 + pow(n_l, 2) * (1 - alpha_2));

        return 0.5 / (lv + vl);
}

// (9.34)
vec3 shading_ggx_brdf(
        const float roughness,
        const vec3 f0,
        const float n_v,
        const float n_l,
        const float n_h,
        const float h_l)
{
        const float alpha = pow(roughness, 2);
        const float alpha_2 = pow(alpha, 2);

        return shading_ggx_fresnel(f0, h_l) * shading_ggx_g2_combined(alpha_2, n_l, n_v) * shading_ggx_d(alpha_2, n_h);
}

#endif
