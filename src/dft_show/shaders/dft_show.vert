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

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 tex_coord;

out vec2 vs_tex_coord;

void main(void)
{
        gl_Position = vec4(position.xy, 0, 1);
        vs_tex_coord = tex_coord;
}
