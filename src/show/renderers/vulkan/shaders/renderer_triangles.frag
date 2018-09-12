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

layout(binding = 1) uniform UniformBufferObject0
{
        float value_r;
        float value_g;
}
ubo0;

layout(binding = 2) uniform UniformBufferObject1
{
        float value_b;
}
ubo1;

layout(binding = 3) uniform sampler2D texture_sampler;

//

layout(location = 0) in VS
{
        vec2 texture_coordinates;
}
vs;

//

layout(location = 0) out vec4 color;

void main()
{
        // color = vec4(vs.texture_coordinates, 0, 1);

        color = texture(texture_sampler, vs.texture_coordinates);

        color *= 0.5 + 0.5 * vec4(ubo0.value_r, ubo0.value_g, ubo1.value_b, 1);
}
