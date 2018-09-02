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

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

layout(binding = 0) uniform UniformBufferObject
{
        mat4 mvp_matrix;
}
ubo;

layout(location = 0) out vec3 fragment_color;

out gl_PerVertex
{
        vec4 gl_Position;
};

void main()
{
        gl_Position = ubo.mvp_matrix * vec4(position, 0.0, 1.0);
        fragment_color = color;
}
