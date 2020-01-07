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
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinates;

layout(std140, binding = 0) uniform Matrices
{
        mat4 matrix;
        mat4 shadow_matrix;
        vec4 clip_plane_equation;
        bool clip_plane_enabled;
}
matrices;

//

layout(location = 0) out VS
{
        vec3 normal;
        vec4 shadow_position;
        vec3 orig_position;
        vec2 texture_coordinates;
}
vs;

out gl_PerVertex
{
        vec4 gl_Position;
        float gl_ClipDistance[1];
};

void main()
{
        gl_Position = matrices.matrix * vec4(position, 1.0);
        gl_ClipDistance[0] = matrices.clip_plane_enabled ? dot(matrices.clip_plane_equation, vec4(position, 1.0)) : 1;
        vs.shadow_position = matrices.shadow_matrix * vec4(position, 1.0);
        vs.orig_position = position;
        vs.normal = normal;
        vs.texture_coordinates = texture_coordinates;
}
