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

layout(bindless_image, r32i) writeonly uniform iimage2D object_img;

layout(std140, binding = 1) uniform Drawing
{
        vec3 default_color;
        vec3 background_color;
        vec3 light_a;
        bool show_fog;
};

layout(location = 0) out vec4 color;

vec3 fog(vec3 fog_color, vec3 fragment_color)
{
        float fog_density = 2;

        float fog_start = 0;
        float fog_end = 1;

        float fog_distance = clamp(gl_FragCoord.z - fog_start, 0, fog_end - fog_start);
        float fog_blending_factor = exp(-fog_density * fog_distance);

        return mix(fog_color, fragment_color, fog_blending_factor);
}

void main(void)
{
        vec3 color3;

        if (show_fog)
        {
                color3 = fog(background_color, default_color * light_a);
        }
        else
        {
                color3 = default_color * light_a;
        }

        color = vec4(color3, 1);

        imageStore(object_img, ivec2(gl_FragCoord.xy), ivec4(1));
}
