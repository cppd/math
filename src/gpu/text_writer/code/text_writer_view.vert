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

#version 450

layout(location = 0) in ivec2 window_coordinates;
layout(location = 1) in vec2 texture_coordinates;

layout(std140, binding = 0) uniform Matrices
{
        mat4 matrix;
}
matrices;

layout(location = 0) out VS
{
        vec2 texture_coordinates;
}
vs;

void main(void)
{
        gl_Position = matrices.matrix * vec4(window_coordinates, 0, 1);
        vs.texture_coordinates = texture_coordinates;
}
