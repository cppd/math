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
#include "mesh_shade.glsl"

layout(early_fragment_tests) in;

layout(location = 0) out vec4 out_color;

void write_transparency(const vec3 color)
{
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
                write_transparency(color);
                discard;
        }
}

void set_fragment_color(const vec3 surface_color, const vec3 n, const vec3 position_for_shadow, const float edge_factor)
{
        const vec3 color = mesh_shade(surface_color, n, position_for_shadow, edge_factor);
        set_fragment_color(color);
}
