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

#ifndef SHADE_GLSL
#define SHADE_GLSL

#include "src/shading/ggx/code/brdf.glsl"
#include "src/shading/ggx/code/metalness.glsl"

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
        const float front_lighting_intensity,
        const float side_lighting_intensity)
{
        const vec3 f0 = shading_ggx_compute_metalness_f0(surface_color, metalness);
        const vec3 rho_ss = shading_ggx_compute_metalness_rho_ss(surface_color, metalness);

        vec3 color = vec3(0);

        if (front_lighting_intensity > 0)
        {
                const vec3 shade = shading_ggx_brdf(
                        roughness, f0, rho_ss, n, v, v, ggx_f1_albedo_cosine_roughness,
                        ggx_f1_albedo_cosine_weighted_average);

                color += front_lighting_intensity * lighting_color * shade;
        }

        if (side_lighting_intensity > 0)
        {
                const vec3 shade = shading_ggx_brdf(
                        roughness, f0, rho_ss, n, v, l, ggx_f1_albedo_cosine_roughness,
                        ggx_f1_albedo_cosine_weighted_average);

                color += side_lighting_intensity * lighting_color * shade;
        }

        return color;
}

#endif
