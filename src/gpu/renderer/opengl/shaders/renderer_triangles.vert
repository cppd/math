/*
Copyright (C) 2017-2019 Topological Manifold

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
layout(location = 2) in vec2 texture_coordinates;
layout(location = 3) in int material_index;
layout(location = 4) in int property;

layout(std140, binding = 0) uniform Matrices
{
        mat4 matrix;
        mat4 shadow_matrix;
};

out VS
{
        vec2 texture_coordinates;
        vec3 normal;
        vec4 shadow_position;
        flat int material_index;
        flat int property;
        flat vec3 orig_position;
}
vs;

void main(void)
{
        gl_Position = matrix * position;

        vs.shadow_position = shadow_matrix * position;

        vs.orig_position = position.xyz;

        vs.normal = normal;
        vs.texture_coordinates = texture_coordinates;
        vs.material_index = material_index;
        vs.property = property;
}
