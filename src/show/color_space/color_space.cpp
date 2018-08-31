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

namespace
{
void convert_color_space(const opengl::ComputeProgram& program, const opengl::TextureRGBA32F& texture)
{
        int groups_x = group_count(texture.texture().width(), GROUP_SIZE);
        int groups_y = group_count(texture.texture().height(), GROUP_SIZE);

        texture.bind_image_texture_read_write(0);

        program.dispatch_compute(groups_x, groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
}

ColorSpaceConverterToRGB::ColorSpaceConverterToRGB() : m_prog(opengl::ComputeShader(color_space_compute_shader))
{
        m_prog.set_uniform("to_rgb", 1);
}

void ColorSpaceConverterToRGB::convert(const opengl::TextureRGBA32F& tex) const
{
        convert_color_space(m_prog, tex);
}

ColorSpaceConverterToSRGB::ColorSpaceConverterToSRGB() : m_prog(opengl::ComputeShader(color_space_compute_shader))
{
        m_prog.set_uniform("to_rgb", 0);
}

void ColorSpaceConverterToSRGB::convert(const opengl::TextureRGBA32F& tex) const
{
        convert_color_space(m_prog, tex);
}
