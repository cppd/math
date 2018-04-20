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

layout(bindless_image, r32i) writeonly uniform iimage2D object_img;

uniform vec4 default_color;
uniform vec4 background_color;

uniform vec4 light_a;

layout(location = 0) out vec4 color;

vec4 fog(vec4 fog_color, vec4 fragment_color)
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
        color = fog(background_color, default_color * light_a);

        imageStore(object_img, ivec2(gl_FragCoord.xy), ivec4(1));
}
