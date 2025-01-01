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

9.8.2 Multiple-Bounce Surface Reflection
*/

#ifndef SHADING_GGX_MULTIPLE_BOUNCE_GLSL
#define SHADING_GGX_MULTIPLE_BOUNCE_GLSL

#include "constants.glsl"
#include "fresnel.glsl"

vec3 shading_ggx_multiple_bounce_surface_reflection(
        const vec3 f0,
        const float rs_f1,
        const float rs_f1_l,
        const float rs_f1_v)
{
        const vec3 f = shading_ggx_fresnel_cosine_weighted_average(f0);

        const vec3 d = f * (rs_f1 * (1 - rs_f1_l) * (1 - rs_f1_v) / (SHADING_GGX_PI * (1 - rs_f1)));

        return d / (vec3(1) - f * (1 - rs_f1));
}

vec3 shading_ggx_multiple_bounce_surface_reflection(
        const vec3 f0,
        const float roughness,
        const float n_l,
        const float n_v,
        const sampler2D ggx_f1_albedo_cosine_roughness,
        const sampler1D ggx_f1_albedo_cosine_weighted_average)
{
        const float rs_f1 = texture(ggx_f1_albedo_cosine_weighted_average, roughness).r;
        const float rs_f1_l = texture(ggx_f1_albedo_cosine_roughness, vec2(n_l, roughness)).r;
        const float rs_f1_v = texture(ggx_f1_albedo_cosine_roughness, vec2(n_v, roughness)).r;

        return shading_ggx_multiple_bounce_surface_reflection(f0, rs_f1, rs_f1_l, rs_f1_v);
}

#endif
