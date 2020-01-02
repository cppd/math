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
#include "com/math.h"
#include "com/vec.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/shader.h"

#include <array>
#include <vector>

constexpr GLenum IMAGE_FORMAT = GL_R32F;

namespace gpu_opengl
{
namespace
{
vec4f color_to_vec4f(const Color& c)
{
        return vec4f(c.red(), c.green(), c.blue(), 1);
}

struct Vertex
{
        static_assert(sizeof(Vector<4, float>) == 4 * sizeof(GLfloat));
        static_assert(sizeof(Vector<2, float>) == 2 * sizeof(GLfloat));

        Vector<4, GLfloat> v; // Конечные координаты вершины
        Vector<2, GLfloat> t; // Координаты вершины в текстуре (0 или 1)
};

class ShaderMemory final
{
        static constexpr int DATA_BINDING = 0;

        opengl::Buffer m_buffer;

        struct Data
        {
                Vector<4, GLfloat> background_color;
                Vector<4, GLfloat> foreground_color;
                GLfloat brightness;
        };

public:
        ShaderMemory() : m_buffer(sizeof(Data), GL_MAP_WRITE_BIT)
        {
        }

        void set_brightness(double brightness) const
        {
                decltype(Data().brightness) b = brightness;
                opengl::map_and_write_to_buffer(m_buffer, offsetof(Data, brightness), b);
        }

        void set_background_color(const Color& color) const
        {
                decltype(Data().background_color) c = color_to_vec4f(color);
                opengl::map_and_write_to_buffer(m_buffer, offsetof(Data, background_color), c);
        }

        void set_foreground_color(const Color& color) const
        {
                decltype(Data().foreground_color) c = color_to_vec4f(color);
                opengl::map_and_write_to_buffer(m_buffer, offsetof(Data, foreground_color), c);
        }

        void bind() const
        {
                glBindBufferBase(GL_UNIFORM_BUFFER, DATA_BINDING, m_buffer);
        }
};

class Impl final : public DFTShow
{
        static constexpr int VERTEX_COUNT = 4;

        opengl::Texture m_result;
        std::unique_ptr<gpu_opengl::DFTComputeTexture> m_dft;
        opengl::VertexArray m_vertex_array;
        std::unique_ptr<opengl::Buffer> m_vertex_buffer;
        opengl::GraphicsProgram m_draw_prog;
        ShaderMemory m_shader_memory;

        int m_dst_x, m_dst_y, m_dst_width, m_dst_height;

        void set_brightness(double brightness) override
        {
                m_shader_memory.set_brightness(brightness);
        }

        void set_background_color(const Color& color) override
        {
                m_shader_memory.set_background_color(color);
        }

        void set_color(const Color& color) override
        {
                m_shader_memory.set_foreground_color(color);
        }

        void draw() override
        {
                m_dft->exec();

                glViewport(m_dst_x, m_dst_y, m_dst_width, m_dst_height);
                m_shader_memory.bind();
                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
        }

public:
        Impl(const opengl::Texture& source, unsigned src_x, unsigned src_y, unsigned src_width, unsigned src_height,
             unsigned dst_x, unsigned dst_y, unsigned dst_width, unsigned dst_height, double brightness,
             const Color& background_color, const Color& color)
                : m_result(IMAGE_FORMAT, src_width, src_height),
                  m_dft(gpu_opengl::create_dft_compute_texture(source, src_x, src_y, src_width, src_height, m_result)),
                  m_draw_prog(opengl::VertexShader(dft_show_vert()), opengl::FragmentShader(dft_show_frag())),
                  m_dst_x(dst_x),
                  m_dst_y(dst_y),
                  m_dst_width(dst_width),
                  m_dst_height(dst_height)
        {
                ASSERT(src_width == dst_width && src_height == dst_height);

                m_draw_prog.set_uniform_handle("tex", m_result.texture_handle());

                set_brightness(brightness);
                set_background_color(background_color);
                set_color(color);

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
};
}

std::unique_ptr<DFTShow> create_dft_show(const opengl::Texture& source, unsigned src_x, unsigned src_y, unsigned src_width,
                                         unsigned src_height, unsigned dst_x, unsigned dst_y, unsigned dst_width,
                                         unsigned dst_height, double brightness, const Color& background_color,
                                         const Color& color)
{
        return std::make_unique<Impl>(source, src_x, src_y, src_width, src_height, dst_x, dst_y, dst_width, dst_height,
                                      brightness, background_color, color);
}
}

#endif
