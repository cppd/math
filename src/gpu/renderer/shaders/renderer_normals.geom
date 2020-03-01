/*
Copyright (C) 2017-2020 Topological Manifold

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

#version 450

layout(points) in;
layout(line_strip, max_vertices = 2) out;

//

layout(std140, binding = 0) uniform Matrices
{
        mat4 main_mvp_matrix;
        mat4 main_model_matrix;
        mat4 main_vp_matrix;
        mat4 shadow_mvp_matrix;
        mat4 shadow_mvp_texture_matrix;
        vec4 clip_plane_equation;
        vec4 clip_plane_equation_shadow;
        bool clip_plane_enabled;
}
matrices;

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

//

void line(vec3 from, vec3 to)
{
        vec4 f = vec4(from, 1.0);
        gl_Position = f;
        gl_ClipDistance[0] = matrices.clip_plane_enabled ? dot(matrices.clip_plane_equation, f) : 1;
        EmitVertex();

        vec4 t = vec4(to, 1.0);
        gl_Position = t;
        gl_ClipDistance[0] = matrices.clip_plane_enabled ? dot(matrices.clip_plane_equation, t) : 1;
        EmitVertex();

        EndPrimitive();
}

void main()
{
        const float LENGTH = 0.01;

        const vec3 from = (matrices.main_mvp_matrix * vec4(vs[0].position, 1.0)).xyz;

        const vec3 world_normal = vs[0].normal;
        const vec3 world_normal_vector = LENGTH * normalize(world_normal);
        const vec3 normal_vector = (matrices.main_vp_matrix * vec4(world_normal_vector, 1)).xyz;

        line(from, from + normal_vector);
}
