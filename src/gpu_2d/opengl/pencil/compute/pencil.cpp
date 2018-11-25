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

#include "pencil.h"

#include "com/math.h"

// clang-format off
constexpr const char pencil_shader[]
{
#include "pencil.comp.str"
};
constexpr const char luminance_rgb_shader[]
{
#include "luminance_rgb.comp.str"
};
// clang-format on

constexpr int GROUP_SIZE = 16;

namespace
{
class Impl final : public PencilEffectGL2D
{
        const int m_groups_x;
        const int m_groups_y;

        const opengl::TextureRGBA32F& m_output;

        opengl::ComputeProgram m_comp_prog;
        opengl::ComputeProgram m_luminance_rgb_prog;

        void exec() override
        {
                m_comp_prog.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Теперь в текстуре находится цвет RGB
                m_output.bind_image_texture_read_write(0);
                m_luminance_rgb_prog.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

public:
        Impl(const opengl::TextureRGBA32F& input, bool input_is_srgb, const opengl::TextureR32I& objects,
             const opengl::TextureRGBA32F& output)
                : m_groups_x(group_count(input.texture().width(), GROUP_SIZE)),
                  m_groups_y(group_count(input.texture().height(), GROUP_SIZE)),
                  m_output(output),
                  m_comp_prog(opengl::ComputeShader(pencil_shader)),
                  m_luminance_rgb_prog(opengl::ComputeShader(luminance_rgb_shader))
        {
                m_comp_prog.set_uniform_handle("img_input", input.image_resident_handle_read_only());
                m_comp_prog.set_uniform_handle("img_output", output.image_resident_handle_write_only());
                m_comp_prog.set_uniform_handle("img_objects", objects.image_resident_handle_read_only());
                m_comp_prog.set_uniform("source_srgb", input_is_srgb);
        }
};
}

std::unique_ptr<PencilEffectGL2D> create_pencil_effect_gl2d(const opengl::TextureRGBA32F& input, bool input_is_srgb,
                                                            const opengl::TextureR32I& objects,
                                                            const opengl::TextureRGBA32F& output)
{
        return std::make_unique<Impl>(input, input_is_srgb, objects, output);
}
