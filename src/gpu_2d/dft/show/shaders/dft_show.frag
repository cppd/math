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

// uniform sampler2D tex;

layout(bindless_sampler) uniform sampler2D tex;
in vec2 vs_tex_coord;

uniform float brightness;

uniform vec4 background_color;
uniform vec4 color;

out vec4 out_color;

void main(void)
{
        // color = texture(tex, vs_tex_coord);

        float v = texture(tex, vs_tex_coord).r;
        v = clamp(v * brightness, 0, 1);
        out_color = mix(background_color, color, v);
}
