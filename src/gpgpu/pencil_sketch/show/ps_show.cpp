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

#include "ps_show.h"

#include "gpgpu/pencil_sketch/compute/ps_gl2d.h"

// clang-format off
constexpr const char vertex_shader[]
{
#include "ps_show.vert.str"
};
constexpr const char fragment_shader[]
{
#include "ps_show.frag.str"
};
// clang-format on

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

class PencilSketch::Impl final
{
        opengl::GraphicsProgram m_draw_prog;
        opengl::TextureRGBA32F m_texture;

        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;

        std::unique_ptr<PencilSketchGL2D> m_pencil_sketch;

public:
        Impl(const opengl::TextureRGBA32F& source, bool source_is_srgb, const opengl::TextureR32I& objects, const mat4& matrix)
                : m_draw_prog(opengl::VertexShader(vertex_shader), opengl::FragmentShader(fragment_shader)),
                  m_texture(source.texture().width(), source.texture().height()),
                  m_pencil_sketch(create_pencil_sketch_gl2d(source, source_is_srgb, objects, m_texture))
        {
                ASSERT(source.texture().width() == objects.texture().width());
                ASSERT(source.texture().height() == objects.texture().height());

                m_draw_prog.set_uniform_handle("tex", m_texture.texture().texture_resident_handle());

                m_vertex_array.attrib_pointer(0, 4, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t), sizeof(Vertex), true);

                int x0 = 0;
                int y0 = 0;
                int x1 = source.texture().width();
                int y1 = source.texture().height();

                std::array<Vertex, VERTEX_COUNT> vertices;

                // Текстурный 0 находится внизу, поэтому текстурная
                // координата по Y для y0 равна 1, а для y1 равна 0
                vertices[0] = {to_vector<float>(matrix * vec4(x0, y0, 0, 1)), {0, 1}};
                vertices[1] = {to_vector<float>(matrix * vec4(x1, y0, 0, 1)), {1, 1}};
                vertices[2] = {to_vector<float>(matrix * vec4(x0, y1, 0, 1)), {0, 0}};
                vertices[3] = {to_vector<float>(matrix * vec4(x1, y1, 0, 1)), {1, 0}};

                m_vertex_buffer.load_static_draw(vertices);
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;

        void draw()
        {
                m_pencil_sketch->exec();

                // Два треугольника на всё окно с текстурой
                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
        }
};

PencilSketch::PencilSketch(const opengl::TextureRGBA32F& source, bool source_is_srgb, const opengl::TextureR32I& objects,
                           const mat4& matrix)
        : m_impl(std::make_unique<Impl>(source, source_is_srgb, objects, matrix))
{
}

PencilSketch::~PencilSketch() = default;

void PencilSketch::draw()
{
        m_impl->draw();
}
