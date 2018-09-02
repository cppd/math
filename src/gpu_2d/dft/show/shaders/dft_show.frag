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

layout(bindless_sampler) uniform sampler2D tex;

uniform float dft_brightness;
uniform vec4 dft_background_color;
uniform vec4 dft_color;

in vec2 vs_texture_coordinates;

out vec4 color;
void main(void)
{
        float v = dft_brightness * texture(tex, vs_texture_coordinates).r;
        color = mix(dft_background_color, dft_color, clamp(v, 0, 1));
}
