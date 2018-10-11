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
#include "com/font/chars.h"
#include "com/font/font.h"
#include "graphics/opengl/objects.h"
#include "graphics/opengl/query.h"

#include <array>
#include <limits>
#include <unordered_map>

constexpr char DEFAULT_CHAR = ' ';

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
        float t1, t2; // Координаты вершины в текстуре.
};

template <typename T>
constexpr unsigned char_to_int(T c)
{
        static_assert(std::is_same_v<T, char>);
        return static_cast<unsigned char>(c);
}
}

class Text::Impl final
{
        int m_step_y;
        int m_start_x;
        int m_start_y;

        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;
        opengl::GraphicsProgram m_program;
        std::unordered_map<char, FontChar> m_chars;
        std::unique_ptr<opengl::TextureR32F> m_texture;

        const FontChar& char_data(char c, char default_char) const
        {
                auto iter = m_chars.find(c);
                if (iter == m_chars.cend())
                {
                        iter = m_chars.find(default_char);
                        if (iter == m_chars.cend())
                        {
                                error("Error finding character " + to_string(char_to_int(c)) + " and default character " +
                                      to_string(char_to_int(default_char)));
                        }
                }
                return iter->second;
        }

        void draw(int& x, int& y, const std::string& line) const
        {
                for (char c : line)
                {
                        if (c == '\n')
                        {
                                y += m_step_y;
                                x = m_start_x;
                                continue;
                        }

                        const FontChar& fc = char_data(c, DEFAULT_CHAR);

                        int x0 = x + fc.left;
                        int y0 = y - fc.top;
                        int x1 = x0 + fc.width;
                        int y1 = y0 + fc.height;

                        float s0 = fc.texture_x;
                        float t0 = fc.texture_y;
                        float s1 = s0 + fc.texture_width;
                        float t1 = t0 + fc.texture_height;

                        std::array<Vertex, 4> vertices;
                        vertices[0] = {x0, y0, s0, t0};
                        vertices[1] = {x1, y0, s1, t0};
                        vertices[2] = {x0, y1, s0, t1};
                        vertices[3] = {x1, y1, s1, t1};

                        m_vertex_buffer.load_dynamic_draw(vertices);
                        m_program.draw_arrays(GL_TRIANGLE_STRIP, 0, vertices.size());

                        x += fc.advance_x;
                }
        }

public:
        Impl(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
                : m_step_y(step_y),
                  m_start_x(start_x),
                  m_start_y(start_y),
                  m_program(opengl::VertexShader(text_vertex_shader), opengl::FragmentShader(text_fragment_shader))
        {
                m_vertex_array.attrib_i_pointer(0, 2, GL_INT, m_vertex_buffer, offsetof(Vertex, w1), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t1), sizeof(Vertex), true);

                set_color(color);
                set_matrix(matrix);

                //

                int max_size = opengl::max_texture_size();

                Font font(size);
                int width, height;
                std::vector<std::uint_least8_t> pixels;
                create_font_chars(font, max_size, max_size, &m_chars, &width, &height, &pixels);

                m_texture = std::make_unique<opengl::TextureR32F>(width, height, pixels);
                m_program.set_uniform_handle("tex", m_texture->texture().texture_resident_handle());
        }

        void set_color(const Color& color) const
        {
                m_program.set_uniform("text_color", color.to_rgb_vector<float>());
        }

        void set_matrix(const mat4& matrix) const
        {
                m_program.set_uniform_float("matrix", matrix);
        }

        void draw(const std::vector<std::string>& text) const
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

        void draw(const std::string& text) const
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

void Text::draw(const std::vector<std::string>& text) const
{
        m_impl->draw(text);
}

void Text::draw(const std::string& text) const
{
        m_impl->draw(text);
}
