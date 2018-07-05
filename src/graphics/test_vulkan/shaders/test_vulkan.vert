/*
Copyright (C) 2017, 2018 Topological Manifold

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
#extension GL_ARB_separate_shader_objects : enable

const vec2 vertices[3] = vec2[](vec2(0, 0.9), vec2(0.9, -0.9), vec2(-0.9, -0.9));

out gl_PerVertex
{
        vec4 gl_Position;
};

void main()
{
        gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
}
