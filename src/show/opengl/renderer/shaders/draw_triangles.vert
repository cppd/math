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
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in int material_index;
layout(location = 4) in int property;

uniform mat4 mvpMatrix;
uniform mat4 shadowMatrix;

out vec2 vs_tex_coord;
out vec3 vs_normal;
out vec4 vs_shadow_coord;

flat out int vs_material_index;
flat out int vs_property;
flat out vec3 orig_position;

void main(void)
{
        gl_Position = mvpMatrix * position;

        vs_shadow_coord = shadowMatrix * position;

        orig_position = position.xyz;

        vs_normal = normal;
        vs_tex_coord = tex_coord;
        vs_material_index = material_index;
        vs_property = property;
}
