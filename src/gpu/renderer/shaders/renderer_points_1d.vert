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

layout(location = 0) in vec3 position;

layout(std140, set = 0, binding = 0) uniform Matrices
{
        mat4 mvp_matrix;
        mat4 shadow_mvp_matrix;
        mat4 shadow_mvp_texture_matrix;
        vec4 clip_plane_equation;
        vec4 clip_plane_equation_shadow;
        bool clip_plane_enabled;
}
matrices;

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

void main(void)
{
        vec4 pos = matrices.mvp_matrix * vec4(position, 1.0);
        gl_Position = pos;
        gl_ClipDistance[0] = matrices.clip_plane_enabled ? dot(matrices.clip_plane_equation, pos) : 1;
}
