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

#ifndef MESH_SHADE_GLSL
#define MESH_SHADE_GLSL

#include "mesh_in.glsl"
#include "ray_tracing_intersection.glsl"
#include "shade.glsl"

#ifdef RAY_TRACING
float mesh_shadow_transparency(const vec3 world_position)
{
        const vec3 org = world_position;
        const vec3 dir = drawing.direction_to_light;
        const bool intersection =
                !drawing.clip_plane_enabled
                        ? ray_tracing_intersection(org, dir, acceleration_structure)
                        : ray_tracing_intersection(org, dir, acceleration_structure, drawing.clip_plane_equation);
        return intersection ? 0 : 1;
}
#else
float mesh_shadow_transparency(const vec3 world_position)
{
        const vec3 shadow_position = (shadow_matrices.world_to_shadow * vec4(world_position, 1)).xyz;
        const float d = texture(shadow_mapping_texture, shadow_position.xy).r;
        return d <= shadow_position.z ? 0 : 1;
}
#endif

vec3 mesh_shade(
        const vec3 surface_color,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const vec3 world_position,
        const float edge_factor)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        vec3 color;

        if (drawing.show_shadow)
        {
                const float transparency = mesh_shadow_transparency(world_position);

                const Lighting lighting =
                        shade(surface_color, metalness, roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, ambient, transparency);

                color = lighting.front + lighting.side;
        }
        else
        {
                color =
                        shade(surface_color, metalness, roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, ambient);
        }

        if (edge_factor >= 0)
        {
                return mix(drawing.wireframe_color, color, edge_factor);
        }

        return color;
}

#endif
