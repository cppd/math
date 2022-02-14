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

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "mesh_in.glsl"
#include "mesh_out.glsl"
#include "ray_tracing_intersection.glsl"
#include "shade.glsl"

layout(location = 0) in GS
{
        vec3 world_normal;
        vec3 world_position;
#ifndef RAY_TRACING
        vec4 shadow_position;
#endif
        vec2 texture_coordinates;
        vec3 baricentric;
}
gs;

//

bool has_texture_coordinates()
{
        return gs.texture_coordinates[0] > -1e9;
}

vec3 surface_color()
{
        if (!mtl.use_material || !drawing.show_materials)
        {
                return mesh.color;
        }

        if (!has_texture_coordinates() || !mtl.use_texture)
        {
                return mtl.color;
        }

        return texture(material_texture, gs.texture_coordinates).rgb;
}

#ifdef RAY_TRACING
float shadow_weight()
{
        const vec3 org = gs.world_position;
        const vec3 dir = drawing.direction_to_light;
        const bool intersection =
                !drawing.clip_plane_enabled
                        ? ray_tracing_intersection(org, dir, acceleration_structure)
                        : ray_tracing_intersection(org, dir, acceleration_structure, drawing.clip_plane_equation);
        return intersection ? 1 : 0;
}
#else
float shadow_weight()
{
        const float d = texture(shadow_mapping_texture, gs.shadow_position.xy).r;
        return d <= gs.shadow_position.z ? 1 : 0;
}
#endif

vec3 shade()
{
        const vec3 n = normalize(gs.world_normal);
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        return drawing.show_shadow
                       ? shade(surface_color(), mesh.metalness, mesh.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                               ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, mesh.ambient,
                               shadow_weight())
                       : shade(surface_color(), mesh.metalness, mesh.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                               ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, mesh.ambient);
}

float edge_factor()
{
        const vec3 d = 0.5 * fwidth(gs.baricentric);
        const vec3 a = smoothstep(vec3(0), d, gs.baricentric);
        return min(min(a.x, a.y), a.z);
}

void main()
{
        vec3 color = shade();

        if (drawing.show_wireframe)
        {
                color = mix(drawing.wireframe_color, color, edge_factor());
        }

        set_fragment_color(color);
}
