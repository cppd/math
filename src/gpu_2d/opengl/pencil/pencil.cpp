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
constexpr const char luminance_rgb_compute_shader[]
{
#include "luminance_rgb.comp.str"
};
// clang-format on

constexpr int GROUP_SIZE = 16;

constexpr int VERTEX_COUNT = 4;

namespace
{
struct Vertex
{
        static_assert(sizeof(Vector<4, float>) == 4 * sizeof(GLfloat));
        static_assert(sizeof(Vector<2, float>) == 2 * sizeof(GLfloat));

        Vector<4, GLfloat> v; // Конечные координаты вершины
        Vector<2, GLfloat> t; // Координаты вершины в текстуре (0 или 1)
};
}

class PencilEffect::Impl final
{
        const int m_width, m_height;
        const int m_groups_x, m_groups_y;
        opengl::ComputeProgram m_comp_prog;
        opengl::ComputeProgram m_luminance_rgb_prog;
        opengl::GraphicsProgram m_draw_prog;
        opengl::TextureRGBA32F m_texture;

        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;

public:
        Impl(const opengl::TextureRGBA32F& source, bool source_is_srgb, const opengl::TextureR32I& objects, const mat4& matrix)
                : m_width(source.texture().width()),
                  m_height(source.texture().height()),
                  m_groups_x(group_count(m_width, GROUP_SIZE)),
                  m_groups_y(group_count(m_height, GROUP_SIZE)),
                  m_comp_prog(opengl::ComputeShader(pencil_compute_shader)),
                  m_luminance_rgb_prog(opengl::ComputeShader(luminance_rgb_compute_shader)),
                  m_draw_prog(opengl::VertexShader(pencil_vertex_shader), opengl::FragmentShader(pencil_fragment_shader)),
                  m_texture(m_width, m_height)
        {
                ASSERT(source.texture().width() == objects.texture().width());
                ASSERT(source.texture().height() == objects.texture().height());

                m_vertex_array.attrib_pointer(0, 4, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t), sizeof(Vertex), true);

                m_comp_prog.set_uniform_handle("img_input", source.image_resident_handle_read_only());
                m_comp_prog.set_uniform_handle("img_output", m_texture.image_resident_handle_write_only());
                m_comp_prog.set_uniform_handle("img_objects", objects.image_resident_handle_read_only());
                m_comp_prog.set_uniform("source_srgb", source_is_srgb);

                m_draw_prog.set_uniform_handle("tex", m_texture.texture().texture_resident_handle());

                int x0 = 0;
                int y0 = 0;
                int x1 = m_width;
                int y1 = m_height;

                std::array<Vertex, VERTEX_COUNT> vertices;

                // Текстурный 0 находится внизу, поэтому текстурная
                // координата по Y для y0 равна 1, а для y1 равна 0
                vertices[0] = {to_vector<float>(matrix * vec4(x0, y0, 0, 1)), {0, 1}};
                vertices[1] = {to_vector<float>(matrix * vec4(x1, y0, 0, 1)), {1, 1}};
                vertices[2] = {to_vector<float>(matrix * vec4(x0, y1, 0, 1)), {0, 0}};
                vertices[3] = {to_vector<float>(matrix * vec4(x1, y1, 0, 1)), {1, 0}};

                m_vertex_buffer.load_static_draw(vertices);
        }

        void draw()
        {
                m_comp_prog.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Теперь в текстуре находится цвет RGB
                m_texture.bind_image_texture_read_write(0);
                m_luminance_rgb_prog.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Два треугольника на всё окно с текстурой
                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
        }
};

PencilEffect::PencilEffect(const opengl::TextureRGBA32F& source, bool source_is_srgb, const opengl::TextureR32I& objects,
                           const mat4& matrix)
        : m_impl(std::make_unique<Impl>(source, source_is_srgb, objects, matrix))
{
}

PencilEffect::~PencilEffect() = default;

void PencilEffect::draw()
{
        m_impl->draw();
}
