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

#version 460

#extension GL_GOOGLE_include_directive : enable
#include "mesh_in.glsl"

layout(points) in;
layout(line_strip, max_vertices = 2) out;

//

layout(location = 0) in VS
{
        vec3 position;
        vec3 normal;
}
vs[1];

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

layout(location = 0) out GS
{
        vec3 color;
}
gs;

//

void line(const vec3 world_from, const vec3 world_to)
{
        const vec4 from = drawing.vp_matrix * vec4(world_from, 1);
        gl_Position = from;
        gl_ClipDistance[0] = drawing.clip_plane_enabled ? dot(drawing.clip_plane_equation, vec4(world_from, 1)) : 1;
        gs.color = drawing.normal_color_negative;
        EmitVertex();

        const vec4 to = drawing.vp_matrix * vec4(world_to, 1);
        gl_Position = to;
        gl_ClipDistance[0] = drawing.clip_plane_enabled ? dot(drawing.clip_plane_equation, vec4(world_to, 1)) : 1;
        gs.color = drawing.normal_color_positive;
        EmitVertex();

        EndPrimitive();
}

void main()
{
        const vec3 world_from = (mesh.model_matrix * vec4(vs[0].position, 1)).xyz;
        const vec3 world_normal = mesh.normal_matrix * vs[0].normal;
        const vec3 world_normal_vector = drawing.normal_length * normalize(world_normal);

        line(world_from - world_normal_vector, world_from + world_normal_vector);
}
