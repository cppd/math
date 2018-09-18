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

float rgb_to_srgb(float c)
{
        if (c >= 1.0)
        {
                return 1.0;
        }
        if (c >= 0.0031308)
        {
                return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
        }
        if (c > 0.0)
        {
                return c * 12.92;
        }
        return 0.0;
}

vec4 rgb_to_srgb(vec4 c)
{
        return vec4(rgb_to_srgb(c.r), rgb_to_srgb(c.g), rgb_to_srgb(c.b), c.a);
}

layout(set = 1, binding = 0) uniform Material
{
        vec3 Ka;
        vec3 Kd;
        vec3 Ks;
        float Ns;
        bool use_texture_Ka;
        bool use_texture_Kd;
        bool use_texture_Ks;
        bool use_material;
}
material;

layout(set = 1, binding = 1) uniform sampler2D texture_Ka;
layout(set = 1, binding = 2) uniform sampler2D texture_Kd;
layout(set = 1, binding = 3) uniform sampler2D texture_Ks;

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

        if (material.use_texture_Kd)
        {
                color = texture(texture_Kd, vs.texture_coordinates);
        }
        else
        {
                color = vec4(material.Kd, 1);
        }

        color = rgb_to_srgb(color);
}
