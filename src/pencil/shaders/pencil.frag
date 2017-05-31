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

// layout(binding = 0) uniform sampler2D tex;
layout(bindless_sampler) uniform sampler2D tex;

// layout(binding = 1, rgba32f) readonly uniform image2D img;

// in vec2 vs_tex_coord;
out vec4 color;

void main(void)
{
        // color = texture(tex, vs_tex_coord);
        color = texelFetch(tex, ivec2(gl_FragCoord.xy), 0);
        // color = imageLoad(img, ivec2(gl_FragCoord.xy));
}
