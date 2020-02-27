/*
Copyright (C) 2017-2020 Topological Manifold

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

layout(binding = 1) uniform sampler2D tex;

layout(std140, binding = 0) uniform Data
{
        vec4 background_color;
        vec4 foreground_color;
        float brightness;
};

layout(location = 0) in VS
{
        vec2 texture_coordinates;
}
vs;

layout(location = 0) out vec4 color;

void main(void)
{
        float v = brightness * texture(tex, vs.texture_coordinates).r;
        color = mix(background_color, foreground_color, clamp(v, 0, 1));
}
