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

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texture_coordinates;

layout(binding = 0) uniform UniformBufferObject
{
        mat4 mvp_matrix;
}
ubo;

//

layout(location = 0) out VS
{
        vec2 texture_coordinates;
}
vs;

out gl_PerVertex
{
        vec4 gl_Position;
};

void main()
{
        gl_Position = ubo.mvp_matrix * vec4(position, 1.0);
        vs.texture_coordinates = texture_coordinates;
}
