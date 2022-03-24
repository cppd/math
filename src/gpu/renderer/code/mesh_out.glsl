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

#include "mesh_in.glsl"
#include "mesh_shade.glsl"
#include "transparency.glsl"

layout(early_fragment_tests) in;

layout(location = 0) out vec4 out_color;

void write_transparency(
        const vec3 color,
        const float alpha,
        const vec3 n,
        const float metalness,
        const float roughness,
        const float ambient,
        const float edge_factor)
{
        const ivec2 coord = ivec2(gl_FragCoord.xy);
        const uint heads_size = imageAtomicAdd(transparency_heads_size, coord, gl_SampleID, 1);

        if (heads_size < TRANSPARENCY_MAX_NODES)
        {
                const uint node = atomicAdd(transparency_node_counter, 1);

                if (node < drawing.transparency_max_node_count)
                {
                        const uint next_node = imageAtomicExchange(transparency_heads, coord, gl_SampleID, node);
                        transparency_nodes[node] = create_transparency_node(
                                color, alpha, n, metalness, roughness, ambient, edge_factor, gl_FragCoord.z, next_node);
                }
        }
        else if (heads_size == TRANSPARENCY_MAX_NODES)
        {
                atomicAdd(transparency_overload_counter, 1);
        }
        discard;
}

void set_fragment_color(const vec3 color)
{
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));

        if (!TRANSPARENCY_DRAWING)
        {
                out_color = vec4(color, 1);
        }
        else
        {
                write_transparency(color, mesh.alpha, vec3(0), 0, 0, 0, 0);
        }
}

void set_fragment_color(const vec3 surface_color, const vec3 n, const vec3 world_position, const float edge_factor)
{
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));

        if (!TRANSPARENCY_DRAWING)
        {
                const vec3 color = mesh_shade(
                        surface_color, n, mesh.metalness, mesh.roughness, mesh.ambient, world_position, edge_factor);
                out_color = vec4(color, 1);
        }
        else
        {
                const vec3 color = mesh_shade(
                        surface_color, n, mesh.metalness, mesh.roughness, mesh.ambient, world_position, edge_factor);
                write_transparency(color, mesh.alpha, n, mesh.metalness, mesh.roughness, mesh.ambient, edge_factor);
        }
}

#endif
