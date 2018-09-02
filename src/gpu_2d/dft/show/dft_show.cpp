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

#include "dft_show.h"

#include "com/math.h"
#include "gpu_2d/dft/comp/dft_gl2d.h"
#include "graphics/opengl/objects.h"

#include <array>
#include <vector>

// clang-format off
constexpr const char dft_show_vertex_shader[]
{
#include "dft_show.vert.str"
};
constexpr const char dft_show_fragment_shader[]
{
#include "dft_show.frag.str"
};
// clang-format on

namespace
{
struct Vertex
{
        static_assert(sizeof(Vector<4, float>) == 4 * sizeof(GLfloat));
        static_assert(sizeof(Vector<2, float>) == 2 * sizeof(GLfloat));

        Vector<4, GLfloat> v; // Конечные координаты вершины
        Vector<2, GLfloat> t; // Координаты вершины в текстуре (0 или 1)
};

vec4f color_to_vec4f(const Color& c)
{
        return vec4f(c.red(), c.green(), c.blue(), 1);
}
}

class DFTShow::Impl final
{
        static constexpr int VERTEX_COUNT = 4;

        const bool m_source_srgb;
        opengl::TextureRGBA32F m_image_texture;
        std::unique_ptr<IFourierGL2> m_gl_fft;
        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;
        opengl::GraphicsProgram m_draw_prog;

public:
        Impl(int width, int height, const mat4& matrix, bool source_srgb, double brightness, const Color& background_color,
             const Color& color)
                : m_source_srgb(source_srgb),
                  m_image_texture(width, height),
                  m_gl_fft(create_fft_gl2d(width, height, m_image_texture)),
                  m_draw_prog(opengl::VertexShader(dft_show_vertex_shader), opengl::FragmentShader(dft_show_fragment_shader))
        {
                m_vertex_array.attrib_pointer(0, 4, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t), sizeof(Vertex), true);

                m_draw_prog.set_uniform_handle("tex", m_image_texture.texture().texture_resident_handle());

                set_brightness(brightness);
                set_background_color(background_color);
                set_color(color);

                int x0 = 0;
                int y0 = 0;
                int x1 = width;
                int y1 = height;

                std::array<Vertex, VERTEX_COUNT> vertices;

                // Текстурный 0 находится внизу, поэтому текстурная
                // координата по Y для y0 равна 1, а для y1 равна 0
                vertices[0] = {to_vector<float>(matrix * vec4(x0, y0, 0, 1)), {0, 1}};
                vertices[1] = {to_vector<float>(matrix * vec4(x1, y0, 0, 1)), {1, 1}};
                vertices[2] = {to_vector<float>(matrix * vec4(x0, y1, 0, 1)), {0, 0}};
                vertices[3] = {to_vector<float>(matrix * vec4(x1, y1, 0, 1)), {1, 0}};

                m_vertex_buffer.load_static_draw(vertices);
        }

        void set_brightness(double brightness)
        {
                m_draw_prog.set_uniform("dft_brightness", static_cast<float>(brightness));
        }

        void set_background_color(const Color& color)
        {
                m_draw_prog.set_uniform("dft_background_color", color_to_vec4f(color));
        }

        void set_color(const Color& color)
        {
                m_draw_prog.set_uniform("dft_color", color_to_vec4f(color));
        }

        void take_image_from_framebuffer()
        {
                m_image_texture.copy_texture_sub_image();
        }

        void draw()
        {
                m_gl_fft->exec(false, m_source_srgb);

                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
        }
};

DFTShow::DFTShow(int width, int height, const mat4& matrix, bool source_srgb, double brightness, const Color& background_color,
                 const Color& color)
        : m_impl(std::make_unique<Impl>(width, height, matrix, source_srgb, brightness, background_color, color))
{
}

DFTShow::~DFTShow() = default;

void DFTShow::set_brightness(double brightness)
{
        m_impl->set_brightness(brightness);
}

void DFTShow::set_background_color(const Color& color)
{
        m_impl->set_background_color(color);
}

void DFTShow::set_color(const Color& color)
{
        m_impl->set_color(color);
}

void DFTShow::take_image_from_framebuffer()
{
        m_impl->take_image_from_framebuffer();
}

void DFTShow::draw()
{
        m_impl->draw();
}
