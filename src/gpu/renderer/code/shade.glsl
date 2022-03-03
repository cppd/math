/*
Copyright (C) 2017-2022 Topological Manifold

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

#ifndef SHADE_GLSL
#define SHADE_GLSL

#include "shading_ggx_diffuse.glsl"
#include "shading_metalness.glsl"

vec3 shade(
        const vec3 surface_color,
        const float metalness,
        const float roughness,
        const vec3 n,
        const vec3 v,
        const vec3 l,
        const sampler2D ggx_f1_albedo_cosine_roughness,
        const sampler1D ggx_f1_albedo_cosine_weighted_average,
        const vec3 lighting_color,
        const float ambient)
{
        const vec3 f0 = shading_compute_metalness_f0(surface_color, metalness);
        const vec3 rho_ss = shading_compute_metalness_rho_ss(surface_color, metalness);

        const vec3 shade_color = shading_ggx_diffuse(
                roughness, f0, rho_ss, n, v, l, ggx_f1_albedo_cosine_roughness, ggx_f1_albedo_cosine_weighted_average);

        return lighting_color * shade_color + ambient * surface_color;
}

vec3 shade(
        const vec3 surface_color,
        const float metalness,
        const float roughness,
        const vec3 n,
        const vec3 v,
        const vec3 l,
        const sampler2D ggx_f1_albedo_cosine_roughness,
        const sampler1D ggx_f1_albedo_cosine_weighted_average,
        const vec3 lighting_color,
        const float ambient,
        const float shadow_transparency)
{
        const vec3 f0 = shading_compute_metalness_f0(surface_color, metalness);
        const vec3 rho_ss = shading_compute_metalness_rho_ss(surface_color, metalness);

        vec3 shade_color = 0.2
                           * shading_ggx_diffuse(
                                   roughness, f0, rho_ss, n, v, v, ggx_f1_albedo_cosine_roughness,
                                   ggx_f1_albedo_cosine_weighted_average);

        if (shadow_transparency > 0)
        {
                shade_color += (shadow_transparency * 0.8)
                               * shading_ggx_diffuse(
                                       roughness, f0, rho_ss, n, v, l, ggx_f1_albedo_cosine_roughness,
                                       ggx_f1_albedo_cosine_weighted_average);
        }

        return lighting_color * shade_color + ambient * surface_color;
}

#endif
