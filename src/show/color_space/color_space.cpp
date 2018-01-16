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

#include "color_space.h"

#include "com/math.h"

// clang-format off
constexpr const char color_space_compute_shader[]
{
#include "color_space.comp.str"
};
// clang-format on

constexpr int GROUP_SIZE = 16;

ColorSpaceConverter::ColorSpaceConverter(bool to_rgb) : m_prog(ComputeShader(color_space_compute_shader))
{
        m_prog.set_uniform("to_rgb", to_rgb ? 1 : 0);
}

void ColorSpaceConverter::convert(const TextureRGBA32F& tex) const
{
        int groups_x = get_group_count(tex.get_texture().get_width(), GROUP_SIZE);
        int groups_y = get_group_count(tex.get_texture().get_height(), GROUP_SIZE);

        tex.bind_image_texture_read_write(0);

        m_prog.dispatch_compute(groups_x, groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
