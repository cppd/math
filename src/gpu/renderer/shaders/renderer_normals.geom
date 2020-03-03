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

layout(std140, binding = 1) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        vec3 background_color;
        vec3 clip_plane_color;
        float normal_length;
        vec3 normal_color_positive;
        vec3 normal_color_negative;
        float default_ns;
        vec3 light_a;
        vec3 light_d;
        vec3 light_s;
        bool show_materials;
        bool show_wireframe;
        bool show_shadow;
        bool show_fog;
}
drawing;

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

void line(vec3 world_from, vec3 world_to)
{
        const vec4 from = matrices.main_vp_matrix * vec4(world_from, 1.0);
        gl_Position = from;
        gl_ClipDistance[0] = matrices.clip_plane_enabled ? dot(matrices.clip_plane_equation, from) : 1;
        gs.color = drawing.normal_color_negative;
        EmitVertex();

        const vec4 to = matrices.main_vp_matrix * vec4(world_to, 1.0);
        gl_Position = to;
        gl_ClipDistance[0] = matrices.clip_plane_enabled ? dot(matrices.clip_plane_equation, to) : 1;
        gs.color = drawing.normal_color_positive;
        EmitVertex();

        EndPrimitive();
}

void main()
{
        const vec3 world_from = (matrices.main_model_matrix * vec4(vs[0].position, 1.0)).xyz;
        const vec3 world_normal = vs[0].normal;
        const vec3 world_normal_vector = drawing.normal_length * normalize(world_normal);

        line(world_from - world_normal_vector, world_from + world_normal_vector);
}
