/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "mesh_common.glsl"

layout(early_fragment_tests) in;

layout(location = 0) out vec4 out_color;

void set_fragment_color(const vec3 color)
{
        imageStore(object_image, ivec2(gl_FragCoord.xy), uvec4(1));

        if (!TRANSPARENCY_DRAWING)
        {
                out_color = vec4(color, 1);
        }
        else
        {
                const uint heads_size = imageAtomicAdd(transparency_heads_size, ivec2(gl_FragCoord.xy), gl_SampleID, 1);
                if (heads_size < TRANSPARENCY_MAX_NODES)
                {
                        const uint index = atomicAdd(transparency_node_counter, 1);
                        if (index < drawing.transparency_max_node_count)
                        {
                                const uint prev_head = imageAtomicExchange(
                                        transparency_heads, ivec2(gl_FragCoord.xy), gl_SampleID, index);

                                TransparencyNode node;
                                node.color_rg = packUnorm2x16(color.rg);
                                node.color_ba = packUnorm2x16(vec2(color.b, mesh.alpha));
                                node.depth = gl_FragCoord.z;
                                node.next = prev_head;

                                transparency_nodes[index] = node;
                        }
                }
                else
                {
                        if (heads_size == TRANSPARENCY_MAX_NODES)
                        {
                                atomicAdd(transparency_overload_counter, 1);
                        }
                }

                discard;
        }
}
