/*
Copyright (C) 2017-2026 Topological Manifold

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

const vec4 VERTICES[4] = {vec4(-1, 1, 0, 1), vec4(1, 1, 0, 1), vec4(-1, -1, 0, 1), vec4(1, -1, 0, 1)};

// texture (0, 0) is top left
// const vec2 TEXTURE_COORDINATES[4] = {vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 0)};

// layout(location = 0) out VS
// {
//         vec2 texture_coordinates;
// }
// vs;

void main()
{
        const int vertex_index = gl_VertexIndex;

        if (vertex_index < 4)
        {
                gl_Position = VERTICES[vertex_index];
                // vs.texture_coordinates = TEXTURE_COORDINATES[vertex_index];
        }
}
