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

layout(location = 0) in vec3 position;

layout(std140, set = 0, binding = 0) uniform Matrices
{
        mat4 main_mvp_matrix;
        mat4 main_model_matrix;
        mat4 main_vp_matrix;
        mat4 shadow_mvp_texture_matrix;
}
matrices;

layout(std140, set = 0, binding = 1) uniform Drawing
{
        vec3 default_color;
        vec3 wireframe_color;
        vec3 background_color;
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
        bool show_smooth;
        vec3 clip_plane_color;
        vec4 clip_plane_equation;
        bool clip_plane_enabled;
}
drawing;

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

void main(void)
{
        vec4 pos = matrices.main_mvp_matrix * vec4(position, 1.0);
        gl_Position = pos;
        if (drawing.clip_plane_enabled)
        {
                vec4 world = matrices.main_model_matrix * vec4(position, 1.0);
                gl_ClipDistance[0] = dot(drawing.clip_plane_equation, world);
        }
        else
        {
                gl_ClipDistance[0] = 1;
        }
}
