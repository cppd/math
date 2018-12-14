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

layout(std140, binding = 0) uniform Data
{
        mat4 matrix;
        float brightness;
};

layout(std430, binding = 1) buffer Points
{
        vec2 points[];
};

void main(void)
{
#if defined(VULKAN)
        int vertex_index = gl_VertexIndex;
#else
        int vertex_index = gl_VertexID;
#endif

        vec2 s = points[vertex_index];
        gl_Position = matrix * vec4(s.x + 0.5, s.y + 0.5, 0, 1);
}
