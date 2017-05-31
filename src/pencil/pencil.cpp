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
        ComputeProgram m_comp_prog;
        GraphicsProgram m_draw_prog;
        TextureRGBA32F m_texture;

public:
        Impl(const Texture2D& tex, const TextureR32I& tex_objects)
                : m_width(tex.get_width()),
                  m_height(tex.get_height()),
                  m_groups_x(get_group_count(m_width, GROUP_SIZE)),
                  m_groups_y(get_group_count(m_height, GROUP_SIZE)),
                  m_comp_prog(ComputeShader(pencil_compute_shader)),
                  m_draw_prog(VertexShader(pencil_vertex_shader), FragmentShader(pencil_fragment_shader)),
                  m_texture(m_width, m_height)
        {
                m_comp_prog.set_uniform_handle("img_input", tex.get_image_resident_handle_read_only_RGBA32F());
                m_comp_prog.set_uniform_handle("img_output",
                                               m_texture.get_texture().get_image_resident_handle_write_only_RGBA32F());
                m_comp_prog.set_uniform_handle("img_objects", tex_objects.get_image_resident_handle_read_only());

                m_draw_prog.set_uniform_handle("tex", m_texture.get_texture().get_texture_resident_handle());
        }

        void draw()
        {
                m_comp_prog.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Два треугольника на всё окно с текстурой
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
        }
};

PencilEffect::PencilEffect(const Texture2D& tex, const TextureR32I& tex_objects)
        : m_impl(std::make_unique<Impl>(tex, tex_objects))
{
}

PencilEffect::~PencilEffect() = default;

void PencilEffect::draw()
{
        m_impl->draw();
}
