/*
Copyright (C) 2017-2020 Topological Manifold

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

#if defined(OPENGL_FOUND)

#include "show.h"

#include "compute.h"
#include "shader_source.h"

#include "com/container.h"
#include "com/vec.h"
#include "graphics/opengl/shader.h"

constexpr int VERTEX_COUNT = 4;
constexpr GLenum IMAGE_FORMAT = GL_R32F;

namespace gpu_opengl
{
namespace
{
struct Vertex
{
        static_assert(sizeof(Vector<4, float>) == 4 * sizeof(GLfloat));
        static_assert(sizeof(Vector<2, float>) == 2 * sizeof(GLfloat));

        Vector<4, GLfloat> v; // Конечные координаты вершины
        Vector<2, GLfloat> t; // Координаты вершины в текстуре (0 или 1)
};

class Impl final : public PencilSketchShow
{
        opengl::GraphicsProgram m_draw_prog;
        opengl::Texture m_texture;

        opengl::VertexArray m_vertex_array;
        std::unique_ptr<opengl::Buffer> m_vertex_buffer;

        std::unique_ptr<gpu_opengl::PencilSketchCompute> m_pencil_sketch;

        unsigned m_x, m_y, m_width, m_height;

        void draw() override
        {
                m_pencil_sketch->exec();

                // Два треугольника на всё окно с текстурой
                glViewport(m_x, m_y, m_width, m_height);
                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
        }

public:
        Impl(const opengl::Texture& source,
             const opengl::Texture& objects,
             unsigned x,
             unsigned y,
             unsigned width,
             unsigned height)
                : m_draw_prog(
                          opengl::VertexShader(pencil_sketch_show_vert()),
                          opengl::FragmentShader(pencil_sketch_show_frag())),
                  m_texture(IMAGE_FORMAT, width, height),
                  m_pencil_sketch(
                          gpu_opengl::create_pencil_sketch_compute(source, objects, x, y, width, height, m_texture)),
                  m_x(x),
                  m_y(y),
                  m_width(width),
                  m_height(height)
        {
                m_draw_prog.set_uniform_handle("tex", m_texture.texture_handle());

                std::array<Vertex, VERTEX_COUNT> vertices;

                // Текстурный 0 находится внизу
                vertices[0] = {{-1, +1, 0, 1}, {0, 1}};
                vertices[1] = {{+1, +1, 0, 1}, {1, 1}};
                vertices[2] = {{-1, -1, 0, 1}, {0, 0}};
                vertices[3] = {{+1, -1, 0, 1}, {1, 0}};

                m_vertex_buffer = std::make_unique<opengl::Buffer>(data_size(vertices), 0, vertices);

                m_vertex_array.attrib(0, 4, GL_FLOAT, *m_vertex_buffer, offsetof(Vertex, v), sizeof(Vertex));
                m_vertex_array.attrib(1, 2, GL_FLOAT, *m_vertex_buffer, offsetof(Vertex, t), sizeof(Vertex));
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

std::unique_ptr<PencilSketchShow> create_pencil_sketch_show(
        const opengl::Texture& source,
        const opengl::Texture& objects,
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height)
{
        return std::make_unique<Impl>(source, objects, x, y, width, height);
}
}

#endif
