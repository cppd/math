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

#include "mesh_in.glsl"
#include "ray_tracing_intersection.glsl"
#include "shade.glsl"

layout(early_fragment_tests) in;

layout(location = 0) out vec4 out_color;

void set_fragment_color(const vec3 color)
{
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));

        if (!TRANSPARENCY_DRAWING)
        {
                out_color = vec4(color, 1);
                return;
        }

        const uint heads_size = imageAtomicAdd(transparency_heads_size, ivec2(gl_FragCoord.xy), gl_SampleID, 1);

        if (heads_size < TRANSPARENCY_MAX_NODES)
        {
                const uint node_index = atomicAdd(transparency_node_counter, 1);
                if (node_index < drawing.transparency_max_node_count)
                {
                        const uint prev_head = imageAtomicExchange(
                                transparency_heads, ivec2(gl_FragCoord.xy), gl_SampleID, node_index);

                        TransparencyNode node;
                        node.color_rg = packUnorm2x16(color.rg);
                        node.color_ba = packUnorm2x16(vec2(color.b, mesh.alpha));
                        node.depth = gl_FragCoord.z;
                        node.next = prev_head;

                        transparency_nodes[node_index] = node;
                }
        }
        else if (heads_size == TRANSPARENCY_MAX_NODES)
        {
                atomicAdd(transparency_overload_counter, 1);
        }

        discard;
}

#ifdef RAY_TRACING
float shadow_weight(const vec3 world_position)
{
        const vec3 org = world_position;
        const vec3 dir = drawing.direction_to_light;
        const bool intersection =
                !drawing.clip_plane_enabled
                        ? ray_tracing_intersection(org, dir, acceleration_structure)
                        : ray_tracing_intersection(org, dir, acceleration_structure, drawing.clip_plane_equation);
        return intersection ? 1 : 0;
}
#else
float shadow_weight(const vec3 shadow_position)
{
        const float d = texture(shadow_mapping_texture, shadow_position.xy).r;
        return d <= shadow_position.z ? 1 : 0;
}
#endif

void set_color(const vec3 surface_color, const vec3 n, const vec3 position_for_shadow, const float edge_factor)
{
        const vec3 v = drawing.direction_to_camera;
        const vec3 l = drawing.direction_to_light;

        vec3 color;

        if (drawing.show_shadow)
        {
                color =
                        shade(surface_color, mesh.metalness, mesh.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, mesh.ambient,
                              shadow_weight(position_for_shadow));
        }
        else
        {
                color =
                        shade(surface_color, mesh.metalness, mesh.roughness, n, v, l, ggx_f1_albedo_cosine_roughness,
                              ggx_f1_albedo_cosine_weighted_average, drawing.lighting_color, mesh.ambient);
        }

        if (edge_factor >= 0)
        {
                color = mix(drawing.wireframe_color, color, edge_factor);
        }

        set_fragment_color(color);
}
