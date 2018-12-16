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

#if !defined(VULKAN)
layout(bindless_sampler) uniform sampler2D tex;
#else
layout(binding = 0) uniform sampler2D tex;
#endif

layout(location = 0) in vec2 vs_texture_coordinates;

layout(location = 0) out vec4 color;

void main(void)
{
        color = texture(tex, vs_texture_coordinates);
}
