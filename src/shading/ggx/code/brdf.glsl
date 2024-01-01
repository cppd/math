/*
Copyright (C) 2017-2024 Topological Manifold

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

#ifndef SHADING_GGX_BRDF_GLSL
#define SHADING_GGX_BRDF_GLSL

#include "ggx.glsl"
#include "multiple_bounce.glsl"
#include "subsurface.glsl"

vec3 shading_ggx_brdf(
        const float roughness,
        const vec3 f0,
        const vec3 rho_ss,
        const vec3 n,
        const vec3 v,
        const vec3 l,
        const sampler2D ggx_f1_albedo_cosine_roughness,
        const sampler1D ggx_f1_albedo_cosine_weighted_average)
{
        const float n_l = dot(n, l);
        if (n_l <= 0)
        {
                return vec3(0);
        }

        const float n_v = dot(n, v);
        if (n_v <= 0)
        {
                return vec3(0);
        }

        const vec3 h = normalize(l + v);
        const float h_l = dot(h, l);
        const float n_h = dot(n, h);

        const vec3 ggx = shading_ggx_brdf(roughness, f0, n_v, n_l, n_h, h_l);

        const vec3 multiple_bounce = shading_ggx_multiple_bounce_surface_reflection(
                f0, roughness, n_l, n_v, ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average);

        const vec3 diffuse = shading_ggx_diffuse_disney_ws(f0, rho_ss, roughness, n_l, n_v, h_l);

        return n_l * (ggx + multiple_bounce + diffuse);
}

#endif
