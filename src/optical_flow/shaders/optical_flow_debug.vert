/*
Copyright (C) 2017 Topological Manifold

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

// layout (location = 0) in vec4 position;
// layout (location = 1) in vec2 tex_coord;

out vec2 vs_tex_coord;

// clang-format off
const vec4 vertices[4] = vec4[4]
(
        vec4(-1,  1, 0, 1),
        vec4( 1,  1, 0, 1),
        vec4(-1, -1, 0, 1),
        vec4( 1, -1, 0, 1)
);
// clang-format on

// рисование в текстуру перевёрнуто по вертикали,
// поэтому координаты текстуры (0, 0) находятся внизу слева
// const vec2 texture_coord[4] = vec2[4](vec2(0, 1), vec2(1, 1), vec2(0, 0), vec2(1, 0));

void main(void)
{
        if (gl_VertexID < 4)
        {
                gl_Position = vertices[gl_VertexID];
                // vs_tex_coord = texture_coord[gl_VertexID];
        }
}
