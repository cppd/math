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

#include "text.h"

#include "com/error.h"
#include "com/font.h"
#include "graphics/opengl/objects.h"

#include <array>
#include <limits>
#include <unordered_map>

// clang-format off
constexpr const char text_vertex_shader[]
{
#include "text.vert.str"
};
constexpr const char text_fragment_shader[]
{
#include "text.frag.str"
};
// clang-format on

namespace
{
struct Vertex
{
        GLint w1, w2; // Координаты вершины в пространстве экрана.
        GLint t1, t2; // Координаты вершины в углах текстуры (0 или 1).
};

class CharData final
{
        const opengl::TextureR32F texture;

public:
        const int width, height, left, top, advance;
        const GLuint64 texture_handle;

        CharData(const Font::Char& c)
                : texture(c.width, c.height, Span<const unsigned char>(c.image, 1ll * c.width * c.height)),
                  width(c.width),
                  height(c.height),
                  left(c.left),
                  top(c.top),
                  advance(c.advance_x),
                  texture_handle(texture.texture().texture_resident_handle())
        {
        }
};
}

class Text::Impl final
{
        Font m_font;

        int m_step_y;
        int m_start_x;
        int m_start_y;

        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;
        opengl::GraphicsProgram m_program;

        std::unordered_map<char, CharData> m_chars;

        const CharData& char_data(char c)
        {
                auto iter = m_chars.find(c);
                if (iter == m_chars.end())
                {
                        iter = m_chars.try_emplace(c, m_font.render_char(c)).first;
                }

                return iter->second;
        }

        void draw(int& x, int& y, const std::string& line)
        {
                for (char c : line)
                {
                        if (c == '\n')
                        {
                                y += m_step_y;
                                x = m_start_x;
                                continue;
                        }

                        const CharData& cd = char_data(c);

                        m_program.set_uniform_handle("tex", cd.texture_handle);

                        int x0 = x + cd.left;
                        int y0 = y - cd.top;
                        int x1 = x0 + cd.width;
                        int y1 = y0 + cd.height;

                        std::array<Vertex, 4> vertices;
                        vertices[0] = {x0, y0, 0, 0};
                        vertices[1] = {x1, y0, 1, 0};
                        vertices[2] = {x0, y1, 0, 1};
                        vertices[3] = {x1, y1, 1, 1};

                        m_vertex_buffer.load_dynamic_draw(vertices);
                        m_program.draw_arrays(GL_TRIANGLE_STRIP, 0, vertices.size());

                        x += cd.advance;
                }
        }

public:
        Impl(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
                : m_font(size),
                  m_step_y(step_y),
                  m_start_x(start_x),
                  m_start_y(start_y),
                  m_program(opengl::VertexShader(text_vertex_shader), opengl::FragmentShader(text_fragment_shader))
        {
                m_vertex_array.attrib_i_pointer(0, 2, GL_INT, m_vertex_buffer, offsetof(Vertex, w1), sizeof(Vertex), true);
                m_vertex_array.attrib_i_pointer(1, 2, GL_INT, m_vertex_buffer, offsetof(Vertex, t1), sizeof(Vertex), true);
                set_color(color);
                set_matrix(matrix);
        }

        void set_color(const Color& color) const
        {
                m_program.set_uniform("text_color", color.to_rgb_vector<float>());
        }

        void set_matrix(const mat4& matrix) const
        {
                m_program.set_uniform_float("matrix", matrix);
        }

        void draw(const std::vector<std::string>& text)
        {
                opengl::GLEnableAndRestore<GL_BLEND> e;

                m_vertex_array.bind();

                int x = m_start_x;
                int y = m_start_y;

                for (const std::string& s : text)
                {
                        draw(x, y, s);
                }
        }

        void draw(const std::string& text)
        {
                opengl::GLEnableAndRestore<GL_BLEND> e;

                m_vertex_array.bind();

                int x = m_start_x;
                int y = m_start_y;

                draw(x, y, text);
        }
};

Text::Text(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
        : m_impl(std::make_unique<Impl>(size, step_y, start_x, start_y, color, matrix))
{
}

Text::~Text() = default;

void Text::set_color(const Color& color) const
{
        m_impl->set_color(color);
}

void Text::set_matrix(const mat4& matrix) const
{
        m_impl->set_matrix(matrix);
}

void Text::draw(const std::vector<std::string>& text)
{
        m_impl->draw(text);
}

void Text::draw(const std::string& text)
{
        m_impl->draw(text);
}
