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

#ifndef MESH_OUT_GLSL
#define MESH_OUT_GLSL

#include "fragments.glsl"
#include "mesh_in.glsl"

layout(early_fragment_tests) in;

layout(location = 0) out uvec4 out_color_0;
layout(location = 1) out vec4 out_color_1;

void write_opacity(
        const vec3 color,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor,
        const uint normal_directed_to_light)
{
        const float ALPHA = 1;

        const OpacityFragment fragment = create_opacity_fragment(
                color, ALPHA, n, metalness, roughness, ambient, edge_factor, gl_FragCoord.z, normal_directed_to_light);

        out_color_0 = fragment.v_0;
        out_color_1 = fragment.v_1;
}

void write_transparency(
        const vec3 color,
        const float alpha,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor,
        const uint normal_directed_to_light)
{
        const ivec2 coord = ivec2(gl_FragCoord.xy);
        const uint heads_size = imageAtomicAdd(transparency_heads_size, coord, gl_SampleID, 1);

        if (heads_size < TRANSPARENCY_MAX_FRAGMENT_COUNT)
        {
                const uint node = atomicAdd(transparency_node_counter, 1);

                if (node < drawing.transparency_max_node_count)
                {
                        const uint next_node = imageAtomicExchange(transparency_heads, coord, gl_SampleID, node);
                        transparency_nodes[node] = create_transparency_node(
                                color, alpha, n, metalness, roughness, ambient, edge_factor, gl_FragCoord.z,
                                normal_directed_to_light, next_node);
                }
        }
        else if (heads_size == TRANSPARENCY_MAX_FRAGMENT_COUNT)
        {
                atomicAdd(transparency_overload_counter, 1);
        }

        discard;
}

void set_fragment_color(const vec3 color)
{
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));

        const vec3 N = vec3(0);
        const float METALNESS = 0;
        const float ROUGHNESS = 0;
        const float AMBIENT = 0;
        const float EDGE_FACTOR = 0;
        const uint NORMAL_DIRECTED_TO_LIGHT = 0;

        if (!transparency_drawing)
        {
                write_opacity(color, N, METALNESS, ROUGHNESS, AMBIENT, EDGE_FACTOR, NORMAL_DIRECTED_TO_LIGHT);
        }
        else
        {
                write_transparency(
                        color, mesh.alpha, N, METALNESS, ROUGHNESS, AMBIENT, EDGE_FACTOR, NORMAL_DIRECTED_TO_LIGHT);
        }
}

void set_fragment_color(
        const vec3 surface_color,
        const vec3 n,
        const vec3 world_position,
        const float edge_factor,
        const uint normal_directed_to_light)
{
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));

        if (!transparency_drawing)
        {
                write_opacity(
                        surface_color, n, mesh.metalness, mesh.roughness, mesh.ambient, edge_factor,
                        normal_directed_to_light);
        }
        else
        {
                write_transparency(
                        surface_color, mesh.alpha, n, mesh.metalness, mesh.roughness, mesh.ambient, edge_factor,
                        normal_directed_to_light);
        }
}

#endif
