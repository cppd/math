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
constexpr const char pencil_vertex_shader[]
{
#include "pencil.vert.str"
};
constexpr const char pencil_fragment_shader[]
{
#include "pencil.frag.str"
};
constexpr const char pencil_compute_shader[]
{
#include "pencil.comp.str"
};
// clang-format on

constexpr int GROUP_SIZE = 16;

class PencilEffect::Impl final
{
        const int m_width, m_height;
        const int m_groups_x, m_groups_y;
        opengl::ComputeProgram m_comp_prog;
        opengl::GraphicsProgram m_draw_prog;
        opengl::TextureRGBA32F m_texture;

public:
        Impl(const opengl::TextureRGBA32F& tex, const opengl::TextureR32I& tex_objects, bool source_srgb)
                : m_width(tex.texture().width()),
                  m_height(tex.texture().height()),
                  m_groups_x(group_count(m_width, GROUP_SIZE)),
                  m_groups_y(group_count(m_height, GROUP_SIZE)),
                  m_comp_prog(opengl::ComputeShader(pencil_compute_shader)),
                  m_draw_prog(opengl::VertexShader(pencil_vertex_shader), opengl::FragmentShader(pencil_fragment_shader)),
                  m_texture(m_width, m_height)
        {
                m_comp_prog.set_uniform_handle("img_input", tex.image_resident_handle_read_only());
                m_comp_prog.set_uniform_handle("img_output", m_texture.image_resident_handle_write_only());
                m_comp_prog.set_uniform_handle("img_objects", tex_objects.image_resident_handle_read_only());
                m_comp_prog.set_uniform("source_srgb", source_srgb ? 1 : 0);

                m_draw_prog.set_uniform_handle("tex", m_texture.texture().texture_resident_handle());
        }

        void draw()
        {
                m_comp_prog.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Два треугольника на всё окно с текстурой
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
        }
};

PencilEffect::PencilEffect(const opengl::TextureRGBA32F& tex, const opengl::TextureR32I& tex_objects, bool source_srgb)
        : m_impl(std::make_unique<Impl>(tex, tex_objects, source_srgb))
{
}

PencilEffect::~PencilEffect() = default;

void PencilEffect::draw()
{
        m_impl->draw();
}
