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

layout(bindless_image, r32i) writeonly uniform iimage2D object_img;

uniform vec4 default_color;
// uniform vec3 light_direction;
// uniform vec3 camera_direction;

uniform vec4 light_a;

layout(location = 0) out vec4 color;

void main(void)
{
        vec4 color_a = default_color;

        // vec3 L = light_direction;
        // vec3 V = camera_direction;

        color = color_a * light_a;

        imageStore(object_img, ivec2(gl_FragCoord.xy), ivec4(1));
}
